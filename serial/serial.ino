/*
 * ====================================================================
 *   TEST SERIAL COMMAND + LED DEBUGGING (ESP8266)
 * ====================================================================
 */

#define LED_BUILTIN 2 // GPIO2 adalah LED biru bawaan NodeMCU (Active LOW)

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Matikan LED di awal (HIGH = mati untuk GPIO2 ESP8266)
  digitalWrite(LED_BUILTIN, HIGH); 
  
  // KEDIP 3X SEBAGAI TANDA BOOTING / FLASHING SUKSES
  for(int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, LOW);  // Nyala
    delay(150);
    digitalWrite(LED_BUILTIN, HIGH); // Mati
    delay(150);
  }

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
    
    // Nyalakan LED saat data mulai masuk
    digitalWrite(LED_BUILTIN, LOW); 

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
      Serial.println("'");
    }
    
    // Matikan LED setelah selesai memproses dan membalas
    delay(100); 
    digitalWrite(LED_BUILTIN, HIGH); 
  }
}
