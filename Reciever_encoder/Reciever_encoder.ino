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
int indexQuestion = 0;
float diameter = 0.033;

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
      LoRa.beginPacket();
      LoRa.print(input);
      LoRa.endPacket();
      Serial.println("Dato enviado");
    } else {
      //Serial.println("Entrada no válida, por favor ingrese un número.");
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
    indexQuestion = atoi(ptr);
    //Serial.println("Tipo de pregunta: ");
    //Serial.println(indexQuestion);

    if (indexQuestion == 1){
      LoRa.beginPacket();
      LoRa.print(1);
      LoRa.print(",");
      LoRa.print(diameter);
      LoRa.endPacket();
      Serial.println("tipo de encoder enviado");

    } else if (indexQuestion == 2) {
      ptr = strchr(ptr, ',');  // Buscar delimitador
      if (ptr) {
        leftEncoderTicks = atol(++ptr);  // Convertir después de la coma
      }
      ptr = strchr(ptr, ',');  // Buscar delimitador
      if (ptr) {
        batteryLevel = atoi(++ptr);  // Convertir después de la coma
      }
      //Serial.println("datos recibidos desde el emisor");
    }

      // Actualizar OLED si ha pasado suficiente tiempo y hay cambios
      if (currentMillis - lastOledUpdate >= 1000 &&  // Actualizar cada 1 segundo
          (batteryLevel != lastBatteryLevel || leftEncoderTicks != lastEncoderTicks)) {
        lastOledUpdate = currentMillis;  // Actualizar tiempo de la última actualización

        display.clearDisplay();
        display.setCursor(0, 8);
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        if (batteryLevel < 20) {  // Mostrar advertencia de batería baja
          display.println("Batería baja!");
        } else {
          display.print("Batería: ");
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


