#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MPU6050 mpu;

float relative_gx, relative_gy, relative_gz;
float accel_x, accel_y, accel_z;
float descent_velocity;

const char* ssid = "Santiago's A52";
const char* password = "12345678";

WebServer server(80);

Adafruit_BMP280 bmp; // I2C

// Variables de calibración
float gyro_x_cal = 0, gyro_y_cal = 0, gyro_z_cal = 0;
float temp_offset = 0.5;
float humidity_offset = 2.0;
float pressure_offset = 1.25;
float altitude_offset = 0.2;

// Historial de datos (considera limitar el tamaño para evitar problemas de memoria)
String dataHistory = "";

// Límites del historial
const int maxHistoryEntries = 50;
int historyEntries = 0;

// Variables para el estado de los sensores
bool mpuActive = true;

// Variable para la carga de la batería
const int adcPin = 36;  // GPIO 36 (ADC0) en el ESP32
const float R1 = 30000.0; // 30k ohms
const float R2 = 10000.0; // 10k ohms
const float voltageReference = 3.3; // Voltaje de referencia del ADC del ESP32
float batteryVoltage = 0;
int batteryPercentage = 100; // Valor inicial por defecto

// DHT11
#define DHTPIN 18  // Pin donde está conectado el DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float temperature = 0;
float humidity = 0;

String ipAddress;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL

  // Inicializar el MPU6050
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  // Inicializar el DHT11
  dht.begin();

  // Inicializar BMP280
  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }

  // Conectar a WiFi
  Serial.print("Connecting to WiFi ..");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  ipAddress = WiFi.localIP().toString();
  Serial.println("\nConnected to the WiFi network");
  Serial.print("IP Address: ");
  Serial.println(ipAddress);

  // Calibrar el giroscopio
  Serial.println("Calibrating gyroscope... Please keep the sensor stationary for 10 seconds.");
  calibrateGyroscope();

  // Inicializar la pantalla OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while(1);
  }
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();

  // Configurar el servidor web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggleMPU", HTTP_POST, toggleMPU);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  if (mpuActive) {
    static int16_t last_az = 0;
    static unsigned long last_time = 0;

    int16_t ax, ay, az, gx, gy, gz;

    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Calibrate gyroscope data
    relative_gx = (gx - gyro_x_cal) * 250.0 / 32767.0;
    relative_gy = (gy - gyro_y_cal) * 250.0 / 32767.0;
    relative_gz = (gz - gyro_z_cal) * 250.0 / 32767.0;

    accel_x = ax * 9.81 / 16384.0;
    accel_y = ay * 9.81 / 16384.0;
    accel_z = az * 9.81 / 16384.0;

    unsigned long current_time = millis();
    float delta_time = (current_time - last_time) / 1000.0;
    descent_velocity = (az - last_az) * 9.81 / 16384.0 / delta_time;

    last_az = az;
    last_time = current_time;
  }

  // Leer el voltaje de la batería
  int adcValue = analogRead(adcPin);
  float adcVoltage = adcValue / 4095.0 * voltageReference; // Convertir el valor ADC a voltaje
  batteryVoltage = adcVoltage * (R1 + R2) / R2; // Calcular el voltaje de la batería

  // Calcular el porcentaje de batería
  batteryPercentage = (batteryVoltage / 9.0) * 100;

  // Leer datos del DHT11
  temperature = dht.readTemperature() + temp_offset;
  humidity = dht.readHumidity() + humidity_offset;

  // Leer datos del BMP280
  float pressure = bmp.readPressure() / 100.0F + pressure_offset;
  float altitude = bmp.readAltitude(1013.25) + altitude_offset;

  // Save data to history
  String timeStamp = String(millis() / 1000);
  String historyEntry = timeStamp + "s: gx=" + String(relative_gx) + ", gy=" + String(relative_gy) + ", gz=" + String(relative_gz);
  historyEntry += ", ax=" + String(accel_x) + " m/s^2, ay=" + String(accel_y) + " m/s^2, az=" + String(accel_z) + " m/s^2";
  historyEntry += ", vel descent=" + String(descent_velocity) + " m/s<br>";

  // Gestionar el historial para evitar problemas de memoria
  if (historyEntries >= maxHistoryEntries) {
    int firstEntryEnd = dataHistory.indexOf("<br>") + 4;
    dataHistory = dataHistory.substring(firstEntryEnd);
  } else {
    historyEntries++;
  }

  dataHistory += historyEntry;

  server.handleClient();
  delay(2000);

  // Mostrar en la pantalla OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("IP: ");
  display.println(ipAddress);
  display.print("Battery: ");
  display.print(batteryPercentage);
  display.println("%");
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");
  display.print("Pressure: ");
  display.print(pressure);
  display.println(" hPa");
  display.print("Altitude: ");
  display.print(altitude);
  display.println(" m");
  display.display();
}

void handleRoot() {
  String response = "<h2>Datos en Tiempo Real</h2>";

  if (mpuActive) {
    response += "Gyroscope (Relative Position): gx = " + String(relative_gx) + ", gy = " + String(relative_gy) + ", gz = " + String(relative_gz) + "<br>";
    response += "Acceleration: ax = " + String(accel_x) + " m/s^2, ay = " + String(accel_y) + " m/s^2, az = " + String(accel_z) + " m/s^2<br>";
    response += "Descent Velocity: " + String(descent_velocity) + " m/s<br>";
  } else {
    response += "Gyroscope and Acceleration data not available. Sensor is off.<br>";
  }

  response += "<h2>Control de Sensores</h2>";
  response += "<form action=\"/toggleMPU\" method=\"POST\"><button type=\"submit\">" + String(mpuActive ? "Apagar MPU6050" : "Encender MPU6050") + "</button></form><br>";

  response += "<h2>Historial de Datos</h2>";
  response += "<button onclick=\"toggleHistory()\">Mostrar/Esconder Historial</button>";
  response += "<div id=\"history\" style=\"display:none;\">" + dataHistory + "</div>";

  response += "<h2>Carga de la Batería</h2>";
  response += "Nivel de Batería: " + String(batteryVoltage, 2) + "V<br>";
  response += "Porcentaje de Batería: " + String(batteryPercentage) + "%<br>";

  response += "<h2>Temperatura y Humedad</h2>";
  response += "Temperatura: " + String(temperature) + "C<br>";
  response += "Humedad: " + String(humidity) + "%<br>";

  response += "<h2>Presión y Altitud</h2>";
  response += "Presión: " + String(pressure) + " hPa<br>";
  response += "Altitud: " + String(altitude) + " m<br>";

  response += "<h2>Dirección IP</h2>";
  response += "Conéctese a: " + ipAddress + "<br>";

  response += "<script>"
              "function toggleHistory() {"
              "  var x = document.getElementById('history');"
              "  if (x.style.display === 'none') {"
              "    x.style.display = 'block';"
              "  } else {"
              "    x.style.display = 'none';"
              "  }"
              "}"
              "</script>";

  server.send(200, "text/html", response);
}

void toggleMPU() {
  mpuActive = !mpuActive;
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void calibrateGyroscope() {
  int16_t gx, gy, gz;
  long gyro_x_cal_total = 0, gyro_y_cal_total = 0, gyro_z_cal_total = 0;
  const int calibration_samples = 1000;

  for (int i = 0; i < calibration_samples; i++) {
    mpu.getRotation(&gx, &gy, &gz);
    gyro_x_cal_total += gx;
    gyro_y_cal_total += gy;
    gyro_z_cal_total += gz;
    delay(10);
  }

  gyro_x_cal = gyro_x_cal_total / calibration_samples;
  gyro_y_cal = gyro_y_cal_total / calibration_samples;
  gyro_z_cal = gyro_z_cal_total / calibration_samples;

  Serial.println("Gyroscope calibrated:");
  Serial.print("Gyro X offset: "); Serial.println(gyro_x_cal);
  Serial.print("Gyro Y offset: "); Serial.println(gyro_y_cal);
  Serial.print("Gyro Z offset: "); Serial.println(gyro_z_cal);
}