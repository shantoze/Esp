void setup() {
  // Memulai serial di 115200 bps
  Serial.begin(115200);
  delay(500);
  
  // Pesan tanda berhasil booting
  Serial.println("\r\n[SUKSES] ESP8266 Siap Menerima Perintah!");
}

void loop() {
  // Mengecek apakah ada data masuk dari HP
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); // Membersihkan spasi/enter ekstra

    if (cmd == "ping") {
      Serial.println("Pong! Terhubung dengan baik.");
    } 
    else if (cmd == "status") {
      Serial.println("Status: Board Normal & Stabil.");
    } 
    else if (cmd.length() > 0) {
      Serial.print("Echo: ");
      Serial.println(cmd);
    }
  }
  
  // Mencegah watchdog reset
  yield();
}
