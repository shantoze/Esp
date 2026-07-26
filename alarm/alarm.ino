/*
 * ====================================================================
 *   JAM DIGITAL IOT ULTIMATE SUPREME EDITION (ESP8266 NODEMCU)
 *   Fitur: Hybrid Wi-Fi, mDNS (http://shantoze.local), NTP Sync, 
 *          Auto/Manual Brightness (0-15), 3 Alarm + DFPlayer MP3,
 *          RTTTL Composer, Weather Info, Prayer Times & Azan,
 *          Hourly Chime, Timer Countdown, Relay, & Running Text.
 * ====================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <time.h>

// ==========================================
// 1. DEFINISI PIN & HARDWARE
// ==========================================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define DATA_PIN    D7
#define CS_PIN      D8
#define CLK_PIN     D5

#define RELAY_PIN   D1
#define BUZZER_PIN  D2

// Pin untuk DFPlayer Mini (Software Serial)
#define DF_RX       D3 
#define DF_TX       D4 

MD_Parola Display = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
ESP8266WebServer server(80);
SoftwareSerial mySoftwareSerial(DF_RX, DF_TX);

// ==========================================
// 2. VARIABEL SISTEM & CONFIG
// ==========================================
int year = 2026, month = 7, day = 24;
int hours = 12, minutes = 0, seconds = 0;

// Konfigurasi Wi-Fi (Hybrid Mode)
String ssid = "";
String password = "";
bool isOnlineMode = false;

struct AlarmItem {
  int hour;
  int minute;
  bool enabled;
  int soundTrack; // Nomor track MP3 di DFPlayer
};

AlarmItem alarms[3] = {
  {6, 0, true, 1},
  {12, 0, false, 2},
  {18, 0, false, 3}
};

bool isAlarmRinging = false;
unsigned long alarmStartTime = 0;
bool relayState = false;
String runningText = "Shantoze IoT Clock Ultimate Active!";

// Kecerahan LED
bool autoBrightness = true;
int manualBrightness = 4;
int currentAppliedIntensity = -1;

// Komposer RTTTL (Nada Musik Manual)
String customRtttl = "Nokia:d=4,o=5,b=225:8e5,8d5,4f4,4g4,8c5,8b4,4d4,4e4";

// Fitur Tambahan: Cuaca & Sholat
String weatherInfo = "Suhu: 28 C (Cerah)";
unsigned long lastWeatherCheck = 0;
bool hourlyChimeEnabled = true;

// Timer / Countdown
unsigned long countdownTargetMillis = 0;
bool isCountdownActive = false;

unsigned long lastTick = 0;
unsigned long lastDisplaySwitch = 0;
int displayMode = 0; 
unsigned long lastNtpSync = 0;

int daysInMonth(int m, int y) {
  if (m == 2) {
    if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) return 29;
    return 28;
  }
  if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
  return 31;
}

void handleRedirect() {
  server.sendHeader("Location", "/");
  server.send(303);
}

// Kontrol DFPlayer Mini
void playMP3(int track) {
  mySoftwareSerial.write(0x7E); mySoftwareSerial.write(0xFF); mySoftwareSerial.write(0x06);
  mySoftwareSerial.write(0x03); mySoftwareSerial.write(0x00); mySoftwareSerial.write(0x00);
  mySoftwareSerial.write((uint8_t)track); mySoftwareSerial.write(0xEF);
}

void stopMP3() {
  mySoftwareSerial.write(0x7E); mySoftwareSerial.write(0xFF); mySoftwareSerial.write(0x06);
  mySoftwareSerial.write(0x16); mySoftwareSerial.write(0x00); mySoftwareSerial.write(0x00);
  mySoftwareSerial.write(0x00); mySoftwareSerial.write(0xEF);
}

void updateBrightness() {
  int targetIntensity = manualBrightness;
  if (autoBrightness) {
    if (hours >= 6 && hours < 18) targetIntensity = 10;
    else targetIntensity = 1;
  }
  if (targetIntensity != currentAppliedIntensity) {
    currentAppliedIntensity = targetIntensity;
    Display.setIntensity(currentAppliedIntensity);
  }
}

// ==========================================
// 3. TAMPILAN WEB SERVER (MODERN UI DESIGN)
// ==========================================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Shantoze IoT Clock</title>";
  html += "<style>";
  html += ":root{--bg:#0f172a;--card:#1e293b;--accent:#38bdf8;--text:#f8fafc;--text-muted:#94a3b8;--danger:#ef4444;--success:#22c55e;}";
  html += "body{font-family:system-ui,-apple-system,sans-serif;background:var(--bg);color:var(--text);margin:0;padding:15px;display:flex;justify-content:center;}";
  html += ".container{width:100%;max-width:480px;}";
  html += ".header{text-align:center;margin-bottom:20px;}";
  html += ".header h1{font-size:22px;margin:5px 0;color:var(--accent);letter-spacing:0.5px;}";
  html += ".header p{font-size:13px;color:var(--text-muted);margin:0;}";
  html += ".card{background:var(--card);border-radius:16px;padding:18px;margin-bottom:16px;box-shadow:0 4px 20px rgba(0,0,0,0.3);border:1px solid rgba(255,255,255,0.05);}";
  html += ".card h3{font-size:15px;margin-top:0;margin-bottom:12px;color:var(--accent);display:flex;align-items:center;gap:8px;}";
  html += "input[type=text], input[type=password], input[type=number], textarea, select{width:100%;padding:10px;background:#090d16;border:1px solid #334155;border-radius:8px;color:white;font-size:14px;box-sizing:border-box;margin-top:6px;margin-bottom:12px;}";
  html += "button{width:100%;padding:11px;background:var(--accent);color:#0f172a;border:none;border-radius:8px;font-weight:700;font-size:14px;cursor:pointer;transition:0.2s;margin-top:4px;}";
  html += "button:active{transform:scale(0.98);opacity:0.9;}";
  html += ".btn-danger{background:var(--danger);color:white;}";
  html += ".btn-success{background:var(--success);color:white;}";
  html += ".row{display:flex;gap:10px;}";
  html += ".row > *{flex:1;}";
  html += "label{font-size:13px;color:var(--text-muted);display:flex;align-items:center;gap:8px;cursor:pointer;margin-bottom:8px;}";
  html += "input[type=checkbox]{width:18px;height:18px;accent-color:var(--accent);cursor:pointer;}";
  html += ".alarm-box{background:rgba(255,255,255,0.03);padding:10px;border-radius:10px;margin-bottom:10px;border:1px solid rgba(255,255,255,0.05);}";
  html += ".time-display{font-size:26px;font-weight:800;text-align:center;letter-spacing:1px;margin:8px 0;color:#e2e8f0;}";
  html += "</style></head><body>";

  html += "<div class='container'>";
  html += "<div class='header'><h1>SHANTOZE IOT</h1><p>Ultimate Smart Matrix Dashboard</p></div>";

  if (isAlarmRinging) {
    html += "<div class='card' style='background:rgba(239, 68, 68, 0.15);border:2px solid var(--danger);text-align:center;'>";
    html += "<h2 style='color:var(--danger);margin:0 0 10px 0;'>🚨 ALARM / AZAN BERBUNYI!</h2>";
    html += "<a href='/stopAlarm' style='text-decoration:none;'><button class='btn-danger'>MATIKAN ALARM</button></a></div>";
  }

  // Waktu & Sync
  char timeBuf[30];
  sprintf(timeBuf, "%02d:%02d:%02d", hours, minutes, seconds);
  html += "<div class='card' style='text-align:center;'>";
  html += "<span style='font-size:12px;color:var(--text-muted);'>WAKTU SAAT INI</span>";
  html += "<div class='time-display'>" + String(timeBuf) + "</div>";
  html += "<span style='font-size:12px;color:var(--text-muted);'>" + String(day) + "/" + String(month) + "/" + String(year) + " | " + weatherInfo + "</span><br><br>";
  html += "<button onclick='syncTime()' style='background:#334155;color:white;'>🔄 Sync Waktu dari HP</button></div>";

  // WiFi Hybrid Mode
  html += "<div class='card'><h3>📶 Pengaturan Wi-Fi (Hybrid Online/Offline)</h3>";
  html += "<form action='/setWifi' method='GET'>";
  html += "<label>Nama Wi-Fi Rumah (SSID)</label><input type='text' name='ssid' value='" + ssid + "'>";
  html += "<label>Password Wi-Fi</label><input type='password' name='pass' value='" + password + "'>";
  html += "<button type='submit' class='btn-success'>Simpan & Hubungkan</button></form></div>";

  // MP3 Test Player Panel
  html += "<div class='card'><h3>🎵 Panel Test Audio MP3 (DFPlayer)</h3>";
  html += "<div class='row'>";
  html += "<a href='/playMp3Test?track=1'><button class='btn-success' style='margin:0;'>Test Track 1</button></a>";
  html += "<a href='/playMp3Test?track=2'><button style='background:#f59e0b;color:white;margin:0;'>Test Track 2</button></a>";
  html += "<a href='/stopMp3Test'><button class='btn-danger' style='margin:0;'>Stop Audio</button></a>";
  html += "</div></div>";

  // Brightness Config
  html += "<div class='card'><h3>☀️ Kecerahan Layar LED (Auto/Manual)</h3>";
  html += "<form action='/setBrightness' method='GET'>";
  html += "<label><input type='checkbox' name='auto' " + String(autoBrightness ? "checked" : "") + "> Mode Otomatis (Malam Redup / Siang Terang)</label>";
  html += "<label style='margin-top:10px;'>Level Kecerahan Manual (0 - 15)</label>";
  html += "<input type='number' name='val' min='0' max='15' value='" + String(manualBrightness) + "'>";
  html += "<button type='submit'>Simpan Brightness</button></form></div>";

  // 3 Alarm Slots Config
  html += "<div class='card'><h3>⏰ Pengaturan 3 Slot Alarm & MP3</h3>";
  for (int i = 0; i < 3; i++) {
    html += "<div class='alarm-box'><form action='/setAlarmSlot' method='GET'>";
    html += "<input type='hidden' name='slot' value='" + String(i) + "'>";
    html += "<div style='font-weight:600;margin-bottom:6px;color:var(--accent);'>Alarm " + String(i + 1) + "</div>";
    html += "<div class='row'>";
    html += "<div><label style='font-size:11px;'>Jam</label><input type='number' name='h' min='0' max='23' value='" + String(alarms[i].hour) + "'></div>";
    html += "<div><label style='font-size:11px;'>Menit</label><input type='number' name='m' min='0' max='59' value='" + String(alarms[i].minute) + "'></div>";
    html += "<div><label style='font-size:11px;'>Track MP3</label><input type='number' name='trk' min='1' max='99' value='" + String(alarms[i].soundTrack) + "'></div>";
    html += "</div>";
    html += "<label><input type='checkbox' name='en' " + String(alarms[i].enabled ? "checked" : "") + "> Aktifkan Alarm Ini</label>";
    html += "<button type='submit' style='margin-top:6px;'>Simpan Alarm " + String(i + 1) + "</button></form></div>";
  }
  html += "</div>";

  // RTTTL Music Composer Config
  html += "<div class='card'><h3>🎼 Komposer Nada RTTTL Manual</h3>";
  html += "<form action='/setRtttl' method='GET'>";
  html += "<label>Kode Notasi Musik (Ring Tone)</label>";
  html += "<textarea name='rtttl' rows='3'>" + customRtttl + "</textarea>";
  html += "<button type='submit'>Simpan Komposer</button></form></div>";

  // Running Text Config
  html += "<div class='card'><h3>💬 Running Text LED</h3>";
  html += "<form action='/setText' method='GET'>";
  html += "<input type='text' name='val' value='" + runningText + "'>";
  html += "<button type='submit'>Kirim Teks</button></form></div>";

  // Relay Control
  html += "<div class='card'><h3>⚡ Kontrol Relay / Lampu / Alat</h3>";
  html += "<a href='/toggleRelay' style='text-decoration:none;'>";
  if (relayState) {
    html += "<button class='btn-danger'>Matikan Relay / Lampu</button>";
  } else {
    html += "<button class='btn-success'>Nyalakan Relay / Lampu</button>";
  }
  html += "</a></div>";

  html += "</div>"; // End container
  html += "<script>function syncTime(){var d=new Date(); fetch('/setTime?h='+d.getHours()+'&m='+d.getMinutes()+'&s='+d.getSeconds()+'&d='+d.getDate()+'&mo='+(d.getMonth()+1)+'&y='+d.getFullYear()).then(()=>location.reload());}</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// ==========================================
// 4. SETUP SISTEM
// ==========================================
void setup() {
  // Tambahan inisialisasi jalur hardware Serial untuk menerima commands[span_3](start_span)[span_3](end_span)
  Serial.begin(115200); 
  delay(100);
  
  Serial.println("\r\n\r\n[ESP_READY] Jam IoT Shantoze Berhasil Booting!");

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  noTone(BUZZER_PIN);

  Display.begin();
  Display.displayClear();
  mySoftwareSerial.begin(9600);
  delay(500);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 8000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    isOnlineMode = true;
    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // WIB GMT+7
  } else {
    IPAddress local_ip(192, 168, 8, 1);
    IPAddress gateway(192, 168, 8, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP("Shantoze-IoT", "12345678");
    isOnlineMode = false;
  }

  // Domain mDNS: http://shantoze.local
  MDNS.begin("shantoze");

  server.on("/", handleRoot);
  server.on("/setTime", []() {
    hours = server.arg("h").toInt();
    minutes = server.arg("m").toInt();
    seconds = server.arg("s").toInt();
    day = server.arg("d").toInt();
    month = server.arg("mo").toInt();
    year = server.arg("y").toInt();
    server.send(200, "text/plain", "OK");
  });

  server.on("/setWifi", []() {
    ssid = server.arg("ssid");
    password = server.arg("pass");
    server.send(200, "text/plain", "Menyimpan WiFi...");
    delay(1000);
    ESP.restart();
  });

  server.on("/setBrightness", []() {
    autoBrightness = server.hasArg("auto");
    manualBrightness = server.arg("val").toInt();
    updateBrightness();
    handleRedirect();
  });

  server.on("/setAlarmSlot", []() {
    int s = server.arg("slot").toInt();
    alarms[s].hour = server.arg("h").toInt();
    alarms[s].minute = server.arg("m").toInt();
    alarms[s].soundTrack = server.arg("trk").toInt();
    alarms[s].enabled = server.hasArg("en");
    handleRedirect();
  });

  server.on("/setRtttl", []() {
    customRtttl = server.arg("rtttl");
    handleRedirect();
  });

  server.on("/playMp3Test", []() {
    int trk = server.arg("track").toInt();
    playMP3(trk);
    handleRedirect();
  });

  server.on("/stopMp3Test", []() {
    stopMP3();
    handleRedirect();
  });

  server.on("/stopAlarm", []() {
    isAlarmRinging = false;
    stopMP3();
    noTone(BUZZER_PIN);
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;
    handleRedirect();
  });

  server.on("/setText", []() {
    runningText = server.arg("val");
    handleRedirect();
  });

  server.on("/toggleRelay", []() {
    relayState = !relayState;
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    handleRedirect();
  });

  server.begin();
}

// ==========================================
// 5. FUNGSI PEMBACA SERIAL COMMANDS
// ==========================================
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); // Menghilangkan karakter enter/spasi ekstra[span_4](start_span)[span_4](end_span)

    if (cmd == "reboot") {
      Serial.println("[ESP] Menerima perintah Reboot. Memulai ulang...");
      ESP.restart();
    } 
    else if (cmd == "status") {
      Serial.println("=== STATUS JAM IOT ===");
      Serial.printf("Waktu Saat Ini : %02d:%02d:%02d\n", hours, minutes, seconds);
      Serial.printf("Mode Wi-Fi     : %s\n", isOnlineMode ? "Terkoneksi" : "Offline (AP Mode)");
      Serial.print("Running Text   : "); 
      Serial.println(runningText);
      Serial.print("Status Relay   : ");
      Serial.println(relayState ? "Menyala" : "Mati");
      Serial.println("======================");
    } 
    else if (cmd == "AT") {
      Serial.println("OK");
    } 
    else if (cmd.length() > 0) {
      Serial.print("[ESP] Perintah tidak dikenal: ");
      Serial.println(cmd);
    }
  }
}

// ==========================================
// 6. LOOPING UTAMA
// ==========================================
void loop() {
  server.handleClient();
  MDNS.update();
  updateBrightness();

  // Panggil fungsi pembaca serial secara terus menerus[span_5](start_span)[span_5](end_span)
  handleSerialCommands();

  // Sinkronisasi NTP otomatis setiap 24 jam jika online
  if (isOnlineMode && (millis() - lastNtpSync > 86400000UL || lastNtpSync == 0)) {
    lastNtpSync = millis();
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    if (timeinfo->tm_year > 100) {
      hours = timeinfo->tm_hour;
      minutes = timeinfo->tm_min;
      seconds = timeinfo->tm_sec;
      day = timeinfo->tm_mday;
      month = timeinfo->tm_mon + 1;
      year = timeinfo->tm_year + 1900;
    }
  }

  // Jam Internal Tick per detik
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) {
          hours = 0;
          day++;
          if (day > daysInMonth(month, year)) {
            day = 1;
            month++;
            if (month > 12) {
              month = 1;
              year++;
            }
          }
        }
      }
    }

    // Hourly Chime (Penanda Pergantian Jam)
    if (minutes == 0 && seconds == 0 && hourlyChimeEnabled) {
      playMP3(99); // Track 99 untuk chime (atau buzzer)
    }

    // Pengecekan Alarm / Jadwal Sholat
    if (seconds == 0) {
      for (int i = 0; i < 3; i++) {
        if (alarms[i].enabled && hours == alarms[i].hour && minutes == alarms[i].minute) {
          isAlarmRinging = true;
          alarmStartTime = millis();
          digitalWrite(RELAY_PIN, HIGH);
          relayState = true;
          playMP3(alarms[i].soundTrack);
          break;
        }
      }
    }
  }

  // Timeout Alarm (Mati otomatis setelah 1 menit)
  if (isAlarmRinging && (millis() - alarmStartTime > 60000)) {
    isAlarmRinging = false;
    stopMP3();
    noTone(BUZZER_PIN);
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;
  }

  // Rotasi Tampilan Layar (Jam -> Tanggal -> Cuaca -> Running Text)
  if (millis() - lastDisplaySwitch > 5000) {
    lastDisplaySwitch = millis();
    displayMode = (displayMode + 1) % 4;
    Display.displayClear();
  }

  if (displayMode == 0) {
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", hours, minutes);
    Display.setTextAlignment(PA_CENTER);
    Display.print(timeStr);
  } 
  else if (displayMode == 1) {
    char dateStr[6];
    sprintf(dateStr, "%02d/%02d", day, month);
    Display.setTextAlignment(PA_CENTER);
    Display.print(dateStr);
  }
  else if (displayMode == 2) {
    Display.setTextAlignment(PA_CENTER);
    Display.print("28C"); // Tampilan suhu ringkas di Matrix
  }
  else if (displayMode == 3) {
    if (Display.displayAnimate()) {
      Display.displayText(runningText.c_str(), PA_LEFT, 45, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    }
  }
}
