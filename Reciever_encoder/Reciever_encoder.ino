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

char receivedData[256];  // Buffer para datos del paquete
int batteryLevel = 0;
long leftEncoderTicks = 0;
float distanciaRecorrida = 0.0;
int indexQuestion = 0;
float encoderRatio = 0.0;
int encoderType = 0;
float beginReset = 0.0;

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

  Serial.println("Esperando datos por Serial...");
  String odometro_radio = ""; // Variable para almacenar el dato recibido

 /* // Mostrar mensaje de espera
  display.setCursor(0, 23);
  display.print("esperando datos...");
  display.display(); // Mostrar mensaje en pantalla
  */

  // Esperar hasta que llegue un dato por serial
  while (odometro_radio.length() == 0) {
      // Mostrar mensaje de espera
    display.setCursor(0, 23);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("esperando datos...");
    display.display(); // Mostrar mensaje en pantalla
    if (Serial.available() > 0) {
      odometro_radio = Serial.readString(); // Leer el dato como cadena

      display.clearDisplay(); // Limpiar pantalla solo cuando se reciben datos
      display.setCursor(0, 16);
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.print("serial recibido");
      display.display(); // Mostrar en pantalla

      delay(1000); // Pequeño retardo para evitar sobrecarga
    }
  }

  // Procesar los datos recibidos
  int comaIndex = odometro_radio.indexOf(',');
  if (comaIndex != -1) {
    encoderType = odometro_radio.substring(0, comaIndex).toInt();
    encoderRatio = odometro_radio.substring(comaIndex + 1).toFloat();
  }


  // Mostrar los resultados en el display
  display.clearDisplay();
  display.setCursor(0, 8);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print("serial: ");
  display.println(odometro_radio);
  display.setCursor(0, 23);
  display.print("en/rad: ");
  display.print(encoderType);
  display.print(",");
  display.println(encoderRatio);

  display.display();
  delay(5000); // Mostrar los resultados durante 5 segundos
}


void loop() {
  unsigned long currentMillis = millis();  // Obtener tiempo actual

  // Revisar datos del puerto LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    handleLoRaData();
  }

  // Revisar datos del puerto Serial
  if (Serial.available() > 0) {
    handleSerialData();
  }

  // Actualizar la pantalla OLED cada segundo
  if (currentMillis - lastOledUpdate >= 1000) {
    updateOLED();
    lastOledUpdate = currentMillis;
  }

  // Actualizar mensajes periódicos, si aplica
  if (currentMillis - lastPrintMillis >= 1000) {
    lastPrintMillis = currentMillis;
  }
}

// Manejo de datos del LoRa
void handleLoRaData() {
  char receivedData[64];  // Buffer para los datos recibidos
  int i = 0;

  while (LoRa.available() && i < sizeof(receivedData) - 1) {
    receivedData[i++] = LoRa.read();
  }
  receivedData[i] = '\0';  // Terminar la cadena con '\0'

  char* ptr = receivedData;
  indexQuestion = atoi(ptr);  // Determinar tipo de pregunta

  if (indexQuestion == 1) {
    // Enviar configuración del encoder por LoRa
    LoRa.beginPacket();
    LoRa.print(encoderType);
    LoRa.print(",");
    LoRa.print(encoderRatio);
    LoRa.endPacket();
  } else if (indexQuestion == 2) {
    // Procesar datos del paquete recibido
    ptr = strchr(ptr, ',');
    if (ptr) leftEncoderTicks = atol(++ptr);

    ptr = strchr(ptr, ',');
    if (ptr) batteryLevel = atoi(++ptr);

    ptr = strchr(ptr, ',');
    if (ptr) distanciaRecorrida = atof(++ptr);

    Serial.print("ticks:");
    Serial.print(leftEncoderTicks);
    Serial.print(",dist:");
    Serial.println(distanciaRecorrida);
  }
}

// Manejo de datos del Serial
void handleSerialData() {
  String data = Serial.readString();

  if (data.length() > 0) {
    beginReset = data.toFloat();

    // Enviar el dato recibido al LoRa
    LoRa.beginPacket();
    LoRa.print(beginReset);
    int result = LoRa.endPacket(); // Devuelve 0 o 1

    // Determinar el mensaje del resultado
    String envioStatus;
    if (result == 1) {
      envioStatus = "Exitoso";
    } else {
      envioStatus = "Error";
    }

    // Mostrar el dato enviado y el estado en el display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.print("Dato enviado:");
    display.setCursor(0, 10);
    display.println(beginReset);

    display.setCursor(0, 20);
    display.print("Estado envio:");
    display.println(envioStatus);

    display.display();
  }
}



// Actualización del OLED
void updateOLED() {
  display.clearDisplay();
  display.setCursor(0, 8);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (batteryLevel < 20) {
    display.println("Batería baja!");
  } else {
    display.print("Batería: ");
    display.print(batteryLevel);
    display.println("%");
  }

  display.setCursor(0, 16);
  display.print("Distancia: ");
  display.println(distanciaRecorrida);

  display.setCursor(0, 23);
  display.print("Encoder: ");
  display.print(encoderType);
  display.print(",");
  display.println(encoderRatio);

  display.display();
}