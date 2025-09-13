# Smart Greenhouse ESP32
ESP32-based smart greenhouse system using TensorFlow Lite Micro. Collects sensor data (DHT11, MQ135, MQ2, YL95) and controls pump automatically.

> ⚠️ Note: The current code in `src/` is an test version and may not work properly on ESP32, pin assignments in the current code (`DHTPIN`, `MQ135`, `MQ2`, `YL95`, etc.) may be incorrect or differ from your hardware setup. Please verify and adjust the pins before uploading to your ESP32.


## Features
-  **Temperature & Humidity Monitoring** with DHT11  
-  **Soil Moisture Monitoring** with YL95  
-  **Air Quality Monitoring** with MQ135 & MQ2  
-  **ML-based Pump Control** using TensorFlow Lite Micro  
-  **WiFi Integration** – sends sensor data as JSON via REST API  
-  **NTP Time Sync** for accurate timestamps  

---

## Setup with Arduino IDE

1. **Clone the repository**  
   ```bash
   git clone https://github.com/mithatcanbursali/Smart-Greenhouse-ESP32.git
   ```

2. **Open the project in Arduino IDE**  
   - Navigate to `src/main.ino`  
   - Open it in Arduino IDE  

3. **Install required libraries** *(via Library Manager)*  
   - `DHT sensor library` by Adafruit  
   - `TensorFlow Lite for Microcontrollers`  

4. **Update WiFi credentials** in `main.ino`  
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   const char* serverUrl = "http://<server-ip>:5192/api/Sensor/data";
   ```

5. **Select your ESP32 board**  
   - Go to **Tools → Board → ESP32 Dev Module**  
   - Select the correct **Port**  

6. **Upload the code to ESP32**  

7. **Open Serial Monitor** 
   - Go to **Tools → Serial Monitor**  
   - Set **baud rate to 115200**  

---

## Payload Structure
```json
{
  "date": "string",                
  "time": "string",               
  "moisture": "int",              
  "temperature": "float",          
  "airQuality": "int",             
  "gasSensor": "int",              
  "pumperOn": "bool",              
  "heaterOn": "bool",             
  "coolerOn": "bool",              
  "airPurifierOn": "bool",         
  "temperatureCategory": "string", 
  "moistureCategory": "string",    
  "heatingDemand": "float",        
  "coolingDemand": "float",        
  "moistureTemperatureRatio": "float" 
}

```

***PS: heaterOn, coolerOn, and airPurifierOn are placeholders for future features.***
