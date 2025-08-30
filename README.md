# 💊 Smart Medicine Dispenser  

An **ESP32-based IoT Medicine Scheduler & Dispenser** built for the hackathon.  
This project helps patients take medicines on time by dispensing pills automatically from up to **3 different containers**, with reminders and a web interface for scheduling.  

---

## 🚀 Features  
- ⏰ **Time-based scheduling** with NTP synchronization  
- 📱 **Web dashboard** to add, view, and delete medicine schedules    
- 🖥️ **OLED display** showing current medicines and timings    
- 🔄 **Multiple quantity dispensing** with automatic pauses between pills  

---
## ⚙️ How It Works  

1. **WiFi & Time Sync** – The ESP32 connects to WiFi and synchronizes the current time using NTP.  
2. **User Scheduling** – You open the web dashboard and add medicines with name, time, dispenser slot, and quantity.  
3. **Storage** – The schedules are saved in the ESP32’s LittleFS filesystem, so they remain even after a restart.  
4. **Display** – The OLED screen shows the current schedules for all 3 dispensers.  
5. **Automatic Dispensing** – At the scheduled time, the ESP32 activates the corresponding servo to release the set number of pills.  
6. **Feedback** – The device logs activity over Serial, updates the OLED, and refreshes the web UI with the new schedule.  

---
## 🧰 Tech Stack  

### 🔌 Hardware  
- **ESP32 Development Board**  
- **3x Servo Motors** (pill dispensers)  
- **SSD1306 OLED Display (I2C)**  
- **Medicine containers + mechanical mounts**  
- **WiFi network**  

### 💻 Software  
- **Arduino IDE / PlatformIO**  
- **ESP32 Arduino Core**  
- **Libraries:**  
  - WiFi.h  
  - WebServer.h  
  - LittleFS.h  
  - ArduinoJson.h  
  - Adafruit GFX  
  - Adafruit SSD1306  
  - ESP32Servo  

---

## 🙌 Team & Credits
Built with ❤️ by
Jaikhlong Narzary,
Kinjal Boro and
Preetom Boro.
