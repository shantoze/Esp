/*
 * ====================================================================
 *   TEST SERIAL COMMAND MINIMALIS (ESP8266)
 *   Digunakan untuk mengetes komunikasi HP Android <-> ESP8266
 * ====================================================================
 */

void setup() {
  // Aktifkan komunikasi serial di baud rate 115200
  Serial.begin(115200);
  delay(1000);
  
  // Kirim tanda sapaan ke Serial Monitor HP
  Serial.println("\r\n\r\n========================================");
  Serial.println("[TEST] ESP8266 Serial Test Berhasil Booting!");
  Serial.println("[TEST] Silakan ketik 'ping' atau 'status'");
  Serial.println("========================================");
}

void loop() {
  // Mengecek apakah ada data teks masuk dari aplikasi Android
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); // Membersihkan karakter enter/spasi ekstra (\r / \n)

    if (cmd == "ping") {
      Serial.println("[ESP Response] Pong! Koneksi Serial Aman 100%.");
    } 
    else if (cmd == "status") {
      Serial.println("[ESP Response] Status Board: Normal & Siap Digunakan.");
    } 
    else if (cmd.length() > 0) {
      Serial.print("[ESP Response] Perintah tidak dikenal: '");
      Serial.print(cmd);
      Serial.println("' (Tapi teks berhasil diterima ESP!)");
    }
  }
}
