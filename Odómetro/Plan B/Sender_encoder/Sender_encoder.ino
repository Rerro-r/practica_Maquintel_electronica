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
unsigned long lastCommandUpdate = 0;
unsigned long lastCommandAsk = 0;
int batteryLevel = 0;  // Almacenará el nivel de batería
unsigned long lastLoRaSend = 0;  // Tiempo del último paquete LoRa enviado
unsigned long timeBetweenPackets = 0;  // Tiempo entre el último y el actual envío de paquete

char receivedData[256];
char receivedDataSetup[256] = {0};
int encoderType = 0;
float beginReset = 0.0;
float distanciaRecorrida = 0.0;
bool reset = false;
bool stopSending = false;
int ticksNecesarios = 0;
float encoderRatio = 0.0; // medido en metros
float distancia = 0.0;
String runCommand = "";
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


  //#########################################################

  //############# Petición y espera de datos de encoder ######
// Esperar hasta que LoRa esté listo para transmitir
  while (!LoRa.beginPacket()) {  // Bucle hasta que LoRa esté listo
    Serial.println("Esperando a que LoRa esté listo para enviar...");
    delay(100);  // Esperar 100ms antes de intentar nuevamente
  }
  delay(7000);
  // Una vez LoRa esté listo, enviar el paquete


  int packetSize = 0;

  // Enviar el paquete indefinidamente hasta recibir una respuesta
  while (packetSize == 0) {
    LoRa.beginPacket();
    LoRa.print(1);
    LoRa.endPacket(); 
    Serial.println("encoder pedido");
    LoRa.receive();
    // Esperar a que se reciba un paquete
    delay(500);  // Retraso para evitar enviar paquetes demasiado rápido

    packetSize = LoRa.parsePacket();  // Verificar si se ha recibido un paquete
    if (packetSize == 0) {
      Serial.println("Esperando paquete...");
    }
  }

  Serial.println("Paquete recibido.");


  // Leer el paquete recibido en el buffer
  int i = 0;
  while (LoRa.available() && i < sizeof(receivedDataSetup) - 1) {
    receivedDataSetup[i++] = LoRa.read();
  }
  receivedDataSetup[i] = '\0';  // Finalizar la cadena de caracteres

  // Mostrar los datos recibidos
  Serial.println(receivedDataSetup);

  // Procesar los datos recibidos
  char* ptr = receivedDataSetup;
  runCommand = String(ptr);  // Obtener el tipo de encoder (convertir la primera parte a entero)
  ptr = strchr(ptr, ',');   // Buscar el delimitador '/'
  if (ptr) encoderType = atoi(++ptr);  // Convertir la parte después de '/' a flotante
  
  ptr = strchr(ptr, ',');
  if (ptr) encoderRatio = atof(++ptr);

  receivedDataSetup[0] = '\0';  // Esto vacía la cadena
  // Imprimir los valores procesados
  Serial.println("Comando recibido: " + runCommand);
  Serial.println("Tipo de encoder recibido: " + String(encoderType));
  Serial.println("Diámetro del encoder recibido: " + String(encoderRatio));

  

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
  LoRa.receive();
}

void loop() {
  unsigned long currentMillis = millis();
  LoRa.receive();
  // Actualizar la pantalla OLED cada segundo
  if (shouldUpdateDisplay(currentMillis)) {
    updateOLED();
  }

  // Actualizar el nivel de batería cada 5 segundos
  if (shouldUpdateBattery(currentMillis)) {
    updateBatteryLevel();
  }

  if (stopSending == false) {
      // Enviar datos por LoRa
    if (canSendLoRaPacket()) {
      sendLoRaPacket();
    }
  }

  if (shouldUpdateCommand) {
    askCommand();
  }

  if (shouldStopSending) { // falta opción de cuando no le llega nada
    if (LoRa.parsePacket()) {
      Serial.println("Paquete LoRa detectado.");
      readCommand();
    } else {
        Serial.println("No se recibió ningún paquete.");
    }
  }
}


// Verifica si es momento de actualizar la pantalla
bool shouldUpdateDisplay(unsigned long currentMillis) {
  return currentMillis - lastDisplayUpdate >= 1000;
}

bool shouldUpdateCommand(unsigned long currentMillis) {
  return currentMillis - lastCommandUpdate >= 1800000;
}

bool shouldStopSending(unsigned long currentMillis) {
  return currentMillis - lastCommandAsk <= 1000;
}

// Actualiza la pantalla OLED
void updateOLED() {
  lastDisplayUpdate = millis();
  display.clearDisplay();

  bool loraConnected = isLoRaConnected();

  // Mostrar nivel de batería
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
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

  // Mostrar distancia recorrida
  display.setCursor(0, 24);
  display.print("distancia: ");
  distancia = Distance();
  display.print(distancia);

  display.display();
}

// Verifica si es momento de actualizar el nivel de batería
bool shouldUpdateBattery(unsigned long currentMillis) {
  return currentMillis - lastBatteryUpdate >= 5000;
}

void askCommand() {
  if (canSendLoRaPacket()) {
    LoRa.beginPacket();
    LoRa.print(3);
    LoRa.endPacket();
    lastCommandAsk = millis();
    stopSending = true;
  }
}


// Actualiza el nivel de batería
void updateBatteryLevel() {
  lastBatteryUpdate = millis();
  batteryLevel = getBatteryLevel();
}

// Verifica si se puede enviar un paquete LoRa
bool canSendLoRaPacket() {
  return LoRa.beginPacket();
}

// Envía datos por LoRa
void sendLoRaPacket() {
  unsigned long startTime = millis();

  LoRa.beginPacket();
  LoRa.print(2);
  LoRa.print(",");
  LoRa.print(_LeftEncoderTicks);
  LoRa.print(",");
  LoRa.print(batteryLevel);
  LoRa.endPacket();

  // Calcular el tiempo de envío
  unsigned long timeTaken = millis() - startTime;
  timeBetweenPackets = timeTaken;
  lastLoRaSend = millis();
}

// Maneja la recepción de datos LoRa
void readCommand() {
  Serial.println("Comando de respaldo recibido!");

  int i = 0;
  while (LoRa.available() && i < sizeof(receivedData) - 1) {
    receivedData[i++] = LoRa.read();
  }
  receivedData[i] = '\0';  // Terminar la cadena

  // Convertir y procesar el valor recibido
  runCommand = String(receivedData);
  Serial.print("Run Command: ");
  Serial.println(runCommand);
  stopSending = false;
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
