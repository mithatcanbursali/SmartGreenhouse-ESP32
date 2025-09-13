#include <DHT.h>
#include "model.h"

#include <WiFi.h>
#include <HTTPClient.h>

#include <Chirale_TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <time.h>  

// --- Pin Definitions ---
#define DHTPIN 18
#define DHTTYPE DHT11
#define MQ135 33
#define MQ2   34
#define YL95  32

// --- WiFi Credentials (fill with your own) ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "http://<server-ip>:5192/api/Sensor/data";

DHT dht(DHTPIN, DHTTYPE);

// --- TensorFlow Lite Setup ---
const int tensorArenaSize = 8 * 1024; // Increased for safety
uint8_t tensorArena[tensorArenaSize];

const tflite::Model* model;
tflite::MicroInterpreter* interpreter;
TfLiteTensor* input;
TfLiteTensor* output;

const float PUMP_THRESHOLD = 0.5f; // Model output threshold

// --- Helper Functions ---
String getTemperatureCategory(float temp) {
  if (temp < 15) return "low";
  else if (temp < 30) return "medium";
  else return "high";
}

String getMoistureCategory(int moisture) {
  if (moisture < 300) return "dry";
  else if (moisture < 700) return "optimal";
  else return "wet";
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // --- WiFi Connection ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // --- Load Model ---
  model = tflite::GetModel(mlp_model_tflite);
  static tflite::MicroMutableOpResolver<5> resolver;
  resolver.AddFullyConnected();
  resolver.AddRelu();
  resolver.AddLogistic(); 
  resolver.AddSoftmax();  
  resolver.AddReshape();  

  static tflite::MicroInterpreter static_interpreter(model, resolver, tensorArena, tensorArenaSize);
  interpreter = &static_interpreter;

  interpreter->AllocateTensors();
  input = interpreter->input(0);
  output = interpreter->output(0);

  // --- NTP Time Sync ---
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void loop() {
  delay(5000);  

  // --- Sensor Readings ---
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int humidityMapped = map(humidity, 0, 100, 0, 1023);
  int mq135Value = analogRead(MQ135);
  int mq2Value = analogRead(MQ2);
  int moistureValue = analogRead(YL95);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT11 sensor.");
    return;
  }

  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print(" °C, Humidity: "); Serial.print(humidityMapped);
  Serial.print(" %, MQ135: "); Serial.print(mq135Value);
  Serial.print(", MQ2: "); Serial.print(mq2Value);
  Serial.print(", Soil Moisture: "); Serial.println(moistureValue);

  // --- Model Input ---
  input->data.f[0] = 0; // placeholder if not used
  input->data.f[1] = humidityMapped;
  input->data.f[2] = temperature;

  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("Model inference failed!");
    return;
  }

  float result = output->data.f[0];  
  int pump_state = (result > PUMP_THRESHOLD) ? 1 : 0;

  Serial.print("Pump state (model output): ");
  Serial.println(pump_state);

  // --- Optional Heater/Cooler Logic (future expansion) ---
  bool heaterOn = false;
  bool coolerOn = false;
  float heatingDemand = 0.0;
  float coolingDemand = 0.0;

  float moistureTemperatureRatio = (humidityMapped > 0) ? (temperature / humidityMapped) : 0;

  // --- Time ---
  time_t now = time(NULL);
  struct tm* timeinfo = localtime(&now);

  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);

  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

  // --- Categories ---
  String temperatureCategory = getTemperatureCategory(temperature);
  String moistureCategory = getMoistureCategory(humidityMapped);

  // --- JSON Data ---
  String jsonData = "{";
  jsonData += "\"date\":\"" + String(dateStr) + "\",";
  jsonData += "\"time\":\"" + String(timeStr) + "\",";
  jsonData += "\"moisture\":" + String(humidityMapped) + ",";
  jsonData += "\"temperature\":" + String(temperature) + ",";
  jsonData += "\"airQuality\":" + String(mq135Value) + ",";
  jsonData += "\"gasSensor\":" + String(mq2Value) + ",";
  jsonData += "\"pumperOn\":" + String(pump_state == 1 ? "true" : "false") + ",";
  jsonData += "\"heaterOn\":" + String(heaterOn ? "true" : "false") + ",";
  jsonData += "\"coolerOn\":" + String(coolerOn ? "true" : "false") + ",";
  jsonData += "\"airPurifierOn\":false,";
  jsonData += "\"temperatureCategory\":\"" + temperatureCategory + "\",";
  jsonData += "\"moistureCategory\":\"" + moistureCategory + "\",";
  jsonData += "\"heatingDemand\":" + String(heatingDemand, 2) + ",";
  jsonData += "\"coolingDemand\":" + String(coolingDemand, 2) + ",";
  jsonData += "\"moistureTemperatureRatio\":" + String(moistureTemperatureRatio, 4);
  jsonData += "}";

  Serial.println("JSON Sent: " + jsonData);

  // --- Send Data ---
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);  
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    }
    else {
      Serial.println("POST Error: " + String(httpResponseCode));
    }

    http.end();
  }
  else {
    Serial.println("WiFi not connected");
  }

  // --- Pump State Info ---
  if (pump_state > 0) {
    Serial.println("PUMP ON!");
  } else {
    Serial.println("PUMP OFF!");
  }
}
