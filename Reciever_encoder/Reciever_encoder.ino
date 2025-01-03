#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>

//########################### OLED ##############################
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//###############################################################

#define SCK     5
#define MISO    19
#define MOSI    27
#define SS      18
#define RST     23
#define DI0     26
#define BAND    868E6

unsigned long previousMillis = 0;  // Tiempo entre paquetes
unsigned long lastOledUpdate = 0;  // Control de actualización OLED
unsigned long lastPrintMillis = 0;  // Control de impresión en Serial Monitor

char receivedData[50];  // Buffer para datos del paquete
int batteryLevel = 0;
long leftEncoderTicks = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Receiver Optimized");

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Configuración de LoRa para ajustar el rendimiento
  //LoRa.setSpreadingFactor(7);  // Comúnmente 7-12, prueba con 7 para mayor velocidad
  //LoRa.setSignalBandwidth(125E3);  // Ancho de banda de 125 kHz
  //LoRa.setCodingRate4(5);  // Tasa de codificación 4/5


  LoRa.receive();

  if (Serial.available() > 0) {
      // Lee la entrada hasta un salto de línea
      inputString = Serial.readStringUntil("\n");

      // Encuentra la posición de la coma
      int commaIndex = inputString.indexOf(",");

      if (commaIndex != -1) { // Verifica si se encontró la coma
        // Divide la cadena en dos partes usando el índice de la coma
        String value1Str = inputString.substring(0, commaIndex);
        String value2Str = inputString.substring(commaIndex + 1);

        // Convierte las partes en números flotantes
        value1 = value1Str.toFloat();
        value2 = value2Str.toFloat();

        // Muestra los valores en el monitor serial
       // Serial.print("Valor 1: ");
        //Serial.println(value1);
        //Serial.print("Valor 2: ");
        //Serial.println(value2);

  // Verifica si se recibió un número válido
  if (value1 || value2 || Serial.available() > 0) { // Acepta el 0.0 como válido
      Serial.print("Valor 1: ");
      Serial.println(value1);
      Serial.print("Valor 2: ");
      Serial.println(value2);
    // Enviar el valor flotante por LoRa
    
    LoRa.print(value1);
    LoRa.print(",");
    LoRa.print(value2);
    LoRa.print(",");
    LoRa.endPacket();
    Serial.println("Dato enviado");

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  display.clearDisplay();
  display.display();
}

void loop() {

  unsigned long currentMillis = millis();  // Guardar el tiempo actual para el control de intervalos

  //############# Envío de datos ######################

  if (Serial.available() > 0) {
    // Lee el número flotante ingresado por el usuario
    float input = Serial.parseFloat();

    // Verifica si se recibió un número válido
    if (input || Serial.available() > 0) { // Acepta el 0.0 como válido
      Serial.print("Número recibido en el esclavo: ");
      Serial.println(input);  // Verificar en el Serial Monitor del esclavo

      // Enviar el valor flotante por LoRa
      
      LoRa.print(input);
      LoRa.print(",");
      LoRa.print("LOOP");
      LoRa.endPacket();
      Serial.println("Dato enviado");
    } else {
      Serial.println("Entrada no válida, por favor ingrese un número.");
      while (Serial.available() > 0) {
        Serial.read();
      }
    }
  }

  //###################################################

  //############# Recepción de datos ##################
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Leer el paquete en el buffer
    int i = 0;
    while (LoRa.available() && i < sizeof(receivedData) - 1) {
      receivedData[i++] = LoRa.read();
    }
    receivedData[i] = '\0';  // Finalizar cadena

    // Procesar datos con punteros
    static int lastBatteryLevel = -1;
    static long lastEncoderTicks = -1;

    char* ptr = receivedData;
    leftEncoderTicks = atol(ptr);
    ptr = strchr(ptr, ',');  // Buscar delimitador
    if (ptr) {
      batteryLevel = atoi(++ptr);  // Convertir después de la coma

      // Actualizar OLED si ha pasado suficiente tiempo y hay cambios
      if (currentMillis - lastOledUpdate >= 1000 &&  // Actualizar cada 1 segundo
          (batteryLevel != lastBatteryLevel || leftEncoderTicks != lastEncoderTicks)) {
        lastOledUpdate = currentMillis;  // Actualizar tiempo de la última actualización

        display.clearDisplay();
        display.setCursor(0, 8);
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        if (batteryLevel < 20) {  // Mostrar advertencia de batería baja
          display.println("BATTERY LOW!");
        } else {
          display.print("Battery: ");
          display.print(batteryLevel);
          display.println("%");
        }

        display.setCursor(0, 16);
        display.print("Ticks: ");
        display.println(leftEncoderTicks);
        display.display();

        lastBatteryLevel = batteryLevel;
        lastEncoderTicks = leftEncoderTicks;
      }

      // Si es necesario imprimir el tiempo que pasó desde la última recepción
      if (currentMillis - lastPrintMillis >= 1000) {
        lastPrintMillis = currentMillis;
        // Serial.print(interval); // Si quieres imprimir el tiempo
        // Serial.println(" ms");
      }
    }
  }
}

