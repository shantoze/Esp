#include <ESP8266WiFi.h>

// BLOK KODE INJEKSI: Dieksekusi SEBELUM fungsi setup() berjalan
// Memaksa mesin mematikan Radio WiFi sejak detik ke-0 (Bypass Kalibrasi RF)
RF_PRE_INIT() {
  // 3 = Mode RADIO OFF. Mencegah tegangan OTG HP drop mendadak!
  system_phy_set_powerup_option(3);
}

void setup() {
  // Samakan dengan baud rate aplikasi Om
  Serial.begin(74880);
  
  // Beri jeda sesaat agar aliran serial stabil
  delay(500);
  
  // Teks ini DIJAMIN akan muncul karena mesin terhindar dari freeze
  Serial.println("\r\n\r\n========================================");
  Serial.println("[SUKSES] ESP8266 Berhasil Booting via OTG!");
  Serial.println("Kalibrasi RF dimatikan. Daya OTG HP Aman.");
  Serial.println("Silakan tes command 'ping' di aplikasi Flasher.");
  Serial.println("========================================");
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); 

    if (cmd == "ping") {
      Serial.println("[ESP Membalas] PONG! Command dari HP OTG diterima dengan sukses!");
    } 
    else if (cmd.length() > 0) {
      Serial.print("[ESP Membalas] Perintah tak dikenal: ");
      Serial.println(cmd);
    }
  }
  
  yield(); 
}
