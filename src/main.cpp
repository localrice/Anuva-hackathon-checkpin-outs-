// main.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

Servo servo1;
Servo servo2;
Servo servo3;
#define SERVO1_PIN 14    // pin for dispenser 1 servo
#define SERVO2_PIN  27 // pin for dispenser 2 servo
#define SERVO3_PIN  26 // pin for dispenser 3 servo

#define SERVO_STOP 97   // your stop value
#define SERVO_OFFSET 20 // offset for decent speed
// #define ROTATION_MS 1350 // ~1 full rotation
#define S1_ROTATION_MS 1350
#define S2_ROTATION_MS 1800
#define S3_ROTATION_MS 1900

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//
// ========== CONFIG ==========
const char *WIFI_SSID = "no";
const char *WIFI_PASSWORD = "xdtamate69";

#define MAX_MEDICINES 3
const char *NTP_SERVER = "pool.ntp.org";
// Set this to your timezone offset in seconds. Example: IST = +5:30 = 19800
const long GMT_OFFSET_SEC = 19800;
const int DAYLIGHT_OFFSET_SEC = 0; // change if DST active
// ============================

WebServer server(80);

struct Medicine
{
  String name;
  String time;   // "HH:MM"
  int dispenser; // 1..3
  int quantity;  // integer >=1
};

Medicine meds[MAX_MEDICINES];
int medicineCount = 0;

// Keep last triggered minute per slot to prevent repeated triggers in same minute
String lastTriggered[MAX_MEDICINES];

// servo helpers
void stopServo(Servo &s)
{
  s.write(SERVO_STOP);
}


void spinServo(Servo &s, int durationMs) {
  s.write(SERVO_STOP - SERVO_OFFSET);  // spin CW
  delay(durationMs);
  stopServo(s);
}

void dispense(int dispenser, int qty) {
  int duration = 0;

  switch (dispenser) {
    case 1: duration = S1_ROTATION_MS; break;
    case 2: duration = S2_ROTATION_MS; break;
    case 3: duration = S3_ROTATION_MS; break;
    default: return;
  }

  Servo *target = nullptr;
  if (dispenser == 1) target = &servo1;
  else if (dispenser == 2) target = &servo2;
  else if (dispenser == 3) target = &servo3;

  if (!target) return;

  for (int i = 0; i < qty; i++) {
    spinServo(*target, duration);
    delay(500); // small pause between pills
  }
}

// ---------------- LittleFS save/load ----------------
void saveMedicines()
{
  StaticJsonDocument<512> doc;
  for (int i = 0; i < medicineCount; ++i)
  {
    String key = "med" + String(i + 1);
    JsonObject obj = doc.createNestedObject(key);
    obj["name"] = meds[i].name;
    obj["time"] = meds[i].time;
    obj["dispenser"] = meds[i].dispenser;
    obj["quantity"] = meds[i].quantity;
  }

  File f = LittleFS.open("/medicines.json", "w");
  if (!f)
  {
    Serial.println("❌ Failed to open /medicines.json for writing");
    return;
  }
  if (serializeJsonPretty(doc, f) == 0)
  {
    Serial.println("❌ Failed to write JSON");
  }
  else
  {
    Serial.println("✅ Saved medicines to LittleFS");
  }
  f.close();
}

void loadMedicines()
{
  if (!LittleFS.exists("/medicines.json"))
  {
    Serial.println("⚠️ /medicines.json does not exist");
    medicineCount = 0;
    return;
  }

  File f = LittleFS.open("/medicines.json", "r");
  if (!f)
  {
    Serial.println("❌ Failed to open /medicines.json for reading");
    medicineCount = 0;
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err)
  {
    Serial.print("❌ JSON parse failed: ");
    Serial.println(err.c_str());
    medicineCount = 0;
    return;
  }

  medicineCount = 0;
  for (int i = 0; i < MAX_MEDICINES; ++i)
  {
    String key = "med" + String(i + 1);
    if (doc.containsKey(key))
    {
      JsonObject obj = doc[key].as<JsonObject>();
      meds[medicineCount].name = String((const char *)obj["name"]);
      meds[medicineCount].time = String((const char *)obj["time"]);
      meds[medicineCount].dispenser = obj["dispenser"] | 1;
      meds[medicineCount].quantity = obj["quantity"] | 1;
      lastTriggered[medicineCount] = ""; // reset trigger history
      medicineCount++;
    }
  }

  Serial.printf("✅ Loaded %d medicines from LittleFS\n", medicineCount);
}

// ----------------- Helpers -----------------
String htmlEscape(const String &s)
{
  String r = s;
  r.replace("&", "&amp;");
  r.replace("<", "&lt;");
  r.replace(">", "&gt;");
  r.replace("\"", "&quot;");
  r.replace("'", "&#39;");
  return r;
}

String getCurrentHHMM()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return String();
  int hour = timeinfo.tm_hour % 24;
  int minute = timeinfo.tm_min;
  char buf[6];
  sprintf(buf, "%02d:%02d", hour, minute);
  return String(buf);
}

void printCurrentTimeToSerial()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("❌ Time not available yet");
    return;
  }
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  Serial.println(String("🕒 ") + buf);
}

// -- oled screen
void setupOLED()
{
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED allocation failed");
    for (;;)
      ;
  }
  display.clearDisplay();
  display.display();
}

// medicines array should exist: meds[MAX_MEDICINES]
void updateOLED()
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setFont(NULL); // default 5x7 font
  display.setTextSize(1);

  int yStart = 0;     // start from top
  int rowHeight = 22; // taller rows since we now have 2 lines per dispenser

  for (int i = 0; i < 3; i++)
  {
    int y = yStart + i * rowHeight;

    if (i < medicineCount)
    {
      // First line: D#: Name
      display.setCursor(0, y);
      display.print("D");
      display.print(i + 1);
      display.print(": ");
      display.println(meds[i].name);

      // Second line: Time + Quantity
      display.setCursor(10, y + 8); // slight indent for readability
      display.print("T:");
      display.print(meds[i].time);
      display.print("  Q:");
      display.print(meds[i].quantity);
    }
    else
    {
      display.setCursor(0, y);
      display.print("D");
      display.print(i + 1);
      display.println(": --");

      display.setCursor(10, y + 8);
      display.println("T:--:--  Q:--");
    }

    // horizontal line under this row with a 1-pixel gap from text
    int lineY = y + rowHeight - 2; // 1px above row end
    display.drawLine(0, lineY, SCREEN_WIDTH, lineY, SSD1306_WHITE);
  }

  display.display();
}

// ---------------- Web UI ----------------
void handleRoot()
{
  String html = "<!doctype html><html><head><meta charset='utf-8'><title>Medicine Scheduler</title>"
              "<meta name='viewport' content='width=device-width,initial-scale=1'>"
              "<style>"
              "body{font-family:Arial, sans-serif;margin:16px;background:#f9f9f9;color:#222}"
              "h2{text-align:center;margin-bottom:16px}"
              "form{max-width:420px;margin:auto;background:#fff;padding:16px;border:1px solid #ccc;border-radius:8px}"
              "label{display:block;margin:8px 0 4px;font-weight:bold;font-size:14px}"
              "input,select{padding:8px;width:100%;box-sizing:border-box;margin-bottom:12px;border:1px solid #aaa;border-radius:4px}"
              "input[type=submit]{background:#0078d7;color:#fff;border:none;cursor:pointer;font-weight:bold}"
              "input[type=submit]:hover{background:#005fa3}"
              "table{border-collapse:collapse;width:100%;max-width:700px;margin:20px auto;background:#fff;border:1px solid #ccc}"
              "th,td{border:1px solid #ccc;padding:8px;text-align:left;font-size:14px}"
              "th{background:#eee}"
              "td a{color:#d00;text-decoration:none;font-weight:bold}"
              "td a:hover{text-decoration:underline}"
              "p.note{text-align:center;color:#555;font-size:13px;margin-top:16px}"
              "</style></head><body>";

html += "<h2>💊 Medicine Scheduler</h2>";

html += "<form action='/add' method='POST'>";
html += "<label for='name'>Name</label>";
html += "<input type='text' id='name' name='name' required maxlength='40'>";

html += "<label for='time'>Time</label>";
html += "<input type='time' id='time' name='time' required>";

html += "<label for='dispenser'>Dispenser</label>";
html += "<select id='dispenser' name='dispenser'>"
        "<option value='1'>Dispenser 1</option>"
        "<option value='2'>Dispenser 2</option>"
        "<option value='3'>Dispenser 3</option>"
        "</select>";

html += "<label for='quantity'>Quantity</label>";
html += "<input type='number' id='quantity' name='quantity' min='1' max='20' value='1' required>";

html += "<input type='submit' value='Add Medicine'>";
html += "</form><hr>";

// Show current time
String now = getCurrentHHMM();
if (now.length()) {
  html += "<p style='text-align:center;font-weight:bold'>Current time: " + now + "</p>";
} else {
  html += "<p style='text-align:center;font-weight:bold'>Current time: (not available yet)</p>";
}

// List medicines
if (medicineCount == 0) {
  html += "<p style='text-align:center'>No medicines scheduled.</p>";
} else {
  html += "<h3 style='text-align:center'>Scheduled Medicines</h3>";
  html += "<table><tr><th>#</th><th>Name</th><th>Time</th><th>Dispenser</th><th>Qty</th><th>Action</th></tr>";
  for (int i = 0; i < medicineCount; ++i) {
    html += "<tr>";
    html += "<td>" + String(i + 1) + "</td>";
    html += "<td>" + htmlEscape(meds[i].name) + "</td>";
    html += "<td>" + meds[i].time + "</td>";
    html += "<td>" + String(meds[i].dispenser) + "</td>";
    html += "<td>" + String(meds[i].quantity) + "</td>";
    html += "<td><a href='/delete?i=" + String(i) + "' onclick='return confirm(\"Delete this entry?\")'>Delete</a></td>";
    html += "</tr>";
  }
  html += "</table>";
}

html += "<p class='note'>Max " + String(MAX_MEDICINES) + " medicines allowed.</p>";
html += "</body></html>";


  server.send(200, "text/html", html);
}

void handleAdd()
{
  if (!server.hasArg("name") || !server.hasArg("time") || !server.hasArg("dispenser") || !server.hasArg("quantity"))
  {
    server.send(400, "text/plain", "Missing args");
    return;
  }

  if (medicineCount >= MAX_MEDICINES)
  {
    server.send(400, "text/plain", "Max medicines reached");
    return;
  }

  String name = server.arg("name");
  String time = server.arg("time"); // HTML time input -> "HH:MM"
  int dispenser = server.arg("dispenser").toInt();
  int quantity = server.arg("quantity").toInt();
  if (dispenser < 1 || dispenser > 3)
    dispenser = 1;
  if (quantity < 1)
    quantity = 1;

  meds[medicineCount].name = name;
  meds[medicineCount].time = time;
  meds[medicineCount].dispenser = dispenser;
  meds[medicineCount].quantity = quantity;
  lastTriggered[medicineCount] = "";
  medicineCount++;

  saveMedicines();

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDelete()
{
  if (!server.hasArg("i"))
  {
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }
  int idx = server.arg("i").toInt();
  if (idx < 0 || idx >= medicineCount)
  {
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }
  // shift left
  for (int j = idx; j < medicineCount - 1; ++j)
  {
    meds[j] = meds[j + 1];
    lastTriggered[j] = lastTriggered[j + 1];
  }
  medicineCount--;
  lastTriggered[medicineCount] = "";
  saveMedicines();
  server.sendHeader("Location", "/");
  server.send(303);
}

// ---------------- Setup & Loop ----------------
unsigned long lastSerialTimePrint = 0;

void setup()
{
  Serial.begin(115200);
  delay(100);

  // --- Servo setup ---
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servo1.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, 500, 2400);

  servo2.setPeriodHertz(50);
  servo2.attach(SERVO2_PIN, 500, 2400);

  servo3.setPeriodHertz(50);
  servo3.attach(SERVO3_PIN, 500, 2400);

  // stop all at startup
  servo1.write(SERVO_STOP);
  servo2.write(SERVO_STOP);
  servo3.write(SERVO_STOP);

  Wire.begin(23, 19); // SDA, SCL
  setupOLED();
  // Mount LittleFS, auto-format if corrupted
  if (!LittleFS.begin(true))
  {
    Serial.println("❌ LittleFS mount failed");
    // We continue, but filesystem may be unavailable.
  }
  else
  {
    Serial.println("✅ LittleFS mounted");
  }

  loadMedicines();

  // WiFi connect
  Serial.printf("Connecting to WiFi \"%s\" ... ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long st = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - st < 20000)
  { // try 20s
    delay(300);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\n❌ WiFi connect failed (check credentials)");
  }
  else
  {
    Serial.println("\n✅ WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }

  // Configure NTP/time (only works if WiFi connected)
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  // Webserver routes
  server.on("/", handleRoot);
  server.on("/add", HTTP_POST, handleAdd);
  server.on("/delete", handleDelete);
  server.begin();
  Serial.println("✅ Web server started (port 80)");
}

void loop()
{
  server.handleClient();
  static unsigned long lastOLED = 0;
  if (millis() - lastOLED > 1000)
  {
    updateOLED();
    lastOLED = millis();
  }

  // Print current time each second
  if (millis() - lastSerialTimePrint >= 1000)
  {
    printCurrentTimeToSerial();
    lastSerialTimePrint = millis();
  }

  // check medicine times once per loop. Use HH:MM compare
  String now = getCurrentHHMM();
  if (now.length())
  {
    for (int i = 0; i < medicineCount; ++i)
    {
      if (now == meds[i].time)
      {
        // only trigger if not already triggered this minute
        if (lastTriggered[i] != now)
        {
          Serial.printf("⏰ Dispensing: %s | Dispenser %d | Qty %d | Time %s\n",
                        meds[i].name.c_str(), meds[i].dispenser, meds[i].quantity, meds[i].time.c_str());
          // TODO: add dispenser logic// done
          if (meds[i].dispenser == 1)
          {
            for (int q = 0; q < meds[i].quantity; q++)
            {
              dispense(meds[i].dispenser, meds[i].quantity);
              delay(500); // short gap between tablets
            }
          }
          lastTriggered[i] = now;
        }
      }
      else
      {
        // reset lastTriggered when time no longer matches (so it can trigger next day)
        if (lastTriggered[i] == now)
          lastTriggered[i] = "";
      }
    }
  }

  // small yield
  delay(10);
}
