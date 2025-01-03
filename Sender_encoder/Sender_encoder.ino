#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <LoRa.h>

//########################### LORA ##############################
#define SCK     5    // GPIO5  -- SCK
#define MISO    19   // GPIO19 -- MISO
#define MOSI    27   // GPIO27 -- MOSI
#define SS      18   // GPIO18 -- CS
#define RST     23   // GPIO14 -- RESET (If Lora does not work, replace it with GPIO14)
#define DI0     26   // GPIO26 -- IRQ(Interrupt Request)
#define BAND    868E6

String rssi = "RSSI --";
String packSize = "--";
String packet ;
//#########################################################

//########################### OLED ##############################
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//#########################################################

//########################## Encoder ###############################
#define c_LeftEncoderPinA 34
#define c_LeftEncoderPinB 12  
#define LeftEncoderIsReversed
volatile bool _LeftEncoderBSet;
long _LeftEncoderTicks = 0;
String Dato = "";
//#########################################################

//########################## Voltaje ###############################
// Definir el pin ADC que se utilizará
const int batteryPin = 39; // Utiliza un pin ADC como GPIO
// Constantes para el cálculo del voltaje
const float adcResolution = 4095.0;
const float referenceVoltage = 3.3;
const float voltageDividerRatio = 0.41; // Ajustar según tu divisor de voltaje
// Constantes para el mapeo del porcentaje de la batería
const float minBatteryVoltage = 3.3; // Voltaje mínimo de la batería considerada vacía
const float maxBatteryVoltage = 8.0; // Voltaje máximo de la batería considerada llena
const int numReadings = 10; 
//#########################################################

//########################## Variables #############################
unsigned long lastDisplayUpdate = 0;  // Tiempo del último update de pantalla
unsigned long lastBatteryUpdate = 0;  // Tiempo del último cálculo de batería
int batteryLevel = 0;  // Almacenará el nivel de batería
unsigned long lastLoRaSend = 0;  // Tiempo del último paquete LoRa enviado
unsigned long timeBetweenPackets = 0;  // Tiempo entre el último y el actual envío de paquete

char receivedData[50];
int encoderType = 1;
float beginReset = 0.0;
float distanciaRecorrida = 0.0;
bool reset = false;
int ticksNecesarios = 0;
//#########################################################

void setup() {
  Serial.begin(115200);

  //########################### LORA ##############################
  while (!Serial);
  Serial.println();
  Serial.println("LoRa Sender Test");

  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.receive();

  //#########################################################

  //############# Petición y espera de datos de encoder ######

  if (LoRa.beginPacket()) {
    unsigned long startTime = millis();

    LoRa.print(1);
    LoRa.print(",");
    LoRa.print("");
    LoRa.endPacket();
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.println("Paquete recibido desde el esclavo");

    // Leer el paquete en el buffer
    int i = 0;
    while (LoRa.available() && i < sizeof(receivedData) - 1) {
      receivedData[i++] = LoRa.read();
    }
    receivedData[i] = '\0';  // Finalizar cadena

    // Convertir la cadena a un flotante (float)
    if (receivedData != "NO"){
      beginReset = atof(receivedData);
      reset = true;
          // Imprimir el valor flotante recibido en el Serial Monitor
      Serial.print("Valor recibido (beginReset): ");
      Serial.println(beginReset);
    } 

  //##########################################################

  //########################## OLED ###############################
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  //#########################################################

  //########################## ENCODER ###############################
  pinMode(c_LeftEncoderPinA, INPUT_PULLUP); // sets pin A as input with pull-up
  pinMode(c_LeftEncoderPinB, INPUT_PULLUP); // sets pin B as input with pull-up
  attachInterrupt(digitalPinToInterrupt(c_LeftEncoderPinA), HandleLeftMotorInterruptA, RISING);
  //#########################################################
}

void loop() {
  unsigned long currentMillis = millis();

  //########################## OLED ###############################
  if (currentMillis - lastDisplayUpdate >= 1000) {  // Actualiza la pantalla cada 1 segundo
    lastDisplayUpdate = currentMillis;  // Actualiza el tiempo del último update de pantalla

    // Mostrar datos en OLED
    display.clearDisplay();
    bool loraConnected = isLoRaConnected();  // Verifica si LoRa está conectado

    // Mostrar Ticks
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 24);
    display.print("distancia: ");
    display.print(Distance());

    // Mostrar mensaje de batería baja si el nivel está bajo
    display.setCursor(0, 8);
    if (batteryLevel == -1) {
      display.print("Bateria baja!");
    } else {
      display.print("Bateria: ");
      display.print(batteryLevel);
    }

    // Mostrar estado de LoRa
    display.setCursor(0, 16);
    display.print("LoRa: ");
    display.println(loraConnected ? "transmitiendo" : "...");

    display.display();
  }
  //#########################################################

  //########################## Calcular nivel de batería ########################
  if (currentMillis - lastBatteryUpdate >= 5000) {  // Actualiza el nivel de batería cada 5 segundos
    lastBatteryUpdate = currentMillis;

    batteryLevel = getBatteryLevel();  // Calcula el nivel de batería
  }
  //#########################################################

  //########################## Enviar paquete LoRa ##########################
  if (currentMillis - lastLoRaSend >= 2000) {  // Enviar datos cada 2 segundos
    if (LoRa.beginPacket()) {
      unsigned long startTime = millis();

      LoRa.print(_LeftEncoderTicks);
      LoRa.print(",");
      LoRa.print(batteryLevel);
      LoRa.endPacket();

      // Calcular el tiempo que tomó enviar el paquete
      unsigned long timeTaken = millis() - startTime;
      timeBetweenPackets = timeTaken;

      // Imprimir el tiempo de envío en el Serial Monitor
      // Serial.print("Tiempo de envio: ");
      // Serial.print(timeTaken);
      // Serial.println(" ms");

      lastLoRaSend = currentMillis;
    }
  }
  //#########################################################

  //################## Recibir paquete LoRa #################
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.println("Paquete recibido desde el esclavo");

    // Leer el paquete en el buffer
    int i = 0;
    while (LoRa.available() && i < sizeof(receivedData) - 1) {
      receivedData[i++] = LoRa.read();
    }
    receivedData[i] = '\0';  // Finalizar cadena

    // Convertir la cadena a un flotante (float)
    if (receivedData != "NO"){
      beginReset = atof(receivedData);
      reset = true;
          // Imprimir el valor flotante recibido en el Serial Monitor
      Serial.print("Valor recibido (beginReset): ");
      Serial.println(beginReset);
    }  // Convertir el valor recibido a float

  }
}

int getBatteryLevel() {
  long sum = 0;
  for (int i = 0; i < numReadings; i++) {
    sum += analogRead(batteryPin);
    delay(10);  // Pequeña pausa para obtener un valor estable
  }
  int average = sum / numReadings;

  float voltage = (average / 4095.0) * 3.3 / voltageDividerRatio;
  int batteryPercentage = map(voltage * 100, minBatteryVoltage * 100, maxBatteryVoltage * 100, 0, 100);
  batteryPercentage = constrain(batteryPercentage, 0, 100);

  if (batteryPercentage <= 10) {  // Si la batería está muy baja
    return -1;  // "Batería baja"
  } else {
    return batteryPercentage;  // Devuelve el porcentaje de batería
  }
}

bool isLoRaConnected() {
  // Verificar si LoRa está conectado (esto puede requerir un método real de comprobación)
  return true;  // Placeholder para el ejemplo
}

void HandleLeftMotorInterruptA() {
  _LeftEncoderBSet = digitalRead(c_LeftEncoderPinB);   // leer el pin de entrada
  #ifdef LeftEncoderIsReversed
    _LeftEncoderTicks += _LeftEncoderBSet ? -1 : +1;
  #else
    _LeftEncoderTicks -= _LeftEncoderBSet ? -1 : +1;
  #endif
}



float Distance() {
    if (reset == false){
      if (encoderType == 1) { // guía de cable
          distanciaRecorrida = round(((int(_LeftEncoderTicks) * 0.0372 * 3.1416) / 1024) * 1 * 100.0) / 100.0;
      }
      else if (encoderType == 2) { // carrete
          distanciaRecorrida = round(((int(_LeftEncoderTicks) * 0.0225 * 3.1416) / 1024) * 1.0216 * 100.0) / 100.0;
      }
    } else {
      distanciaRecorrida = beginReset;
      reset = false;
      _LeftEncoderTicks = round((distanciaRecorrida * 1024) / (0.0372 * 3.1416));
    }
    
    return distanciaRecorrida;
}

