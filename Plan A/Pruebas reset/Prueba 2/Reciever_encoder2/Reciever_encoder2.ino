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
int encoderType = 3;
float encoderRatio = 0.05;
int indexQuestion = 0;
float distanciaRecorrida = 0.0;

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

    // Verifica si la entrada es válida
    if (!isnan(input)) { // Acepta cualquier número válido, incluyendo 0.0
        Serial.print("Número recibido en el esclavo: ");
        Serial.println(input);  // Verificar en el Serial Monitor del esclavo

        // Enviar el valor flotante por LoRa
        LoRa.print(input);
        LoRa.endPacket();
        Serial.println("Dato enviado");
    } else {
        Serial.println("Entrada no válida, por favor ingrese un número.");
        while (Serial.available() > 0) {
            Serial.read(); // Limpia el buffer del Serial
        }
    }
}




  //###################################################

  //############# Recepción de datos ##################
  // Revisar datos del puerto LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    handleLoRaData();
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
