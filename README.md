# 💊 Smart Medicine Dispenser  

An **ESP32-based IoT Medicine Scheduler & Dispenser** built for the hackathon.  
This project helps patients take medicines on time by dispensing pills automatically from up to **3 different containers**, with reminders and a web interface for scheduling.  

---

## 🚀 Features  
- ⏰ **Time-based scheduling** with NTP synchronization  
- 📱 **Web dashboard** to add, view, and delete medicine schedules  
- 💾 **Persistent storage** using LittleFS (schedules survive reboot)  
- 🖥️ **OLED display** showing current medicines and timings  
- ⚙️ **Three servo-controlled dispensers** for different medicines  
- 🔄 **Multiple quantity dispensing** with automatic pauses between pills  

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
