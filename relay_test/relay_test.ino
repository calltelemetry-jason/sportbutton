// Simple relay test - makes relay click every 2 seconds
// Upload this to test relay functionality

void setup() {
  Serial.begin(115200);
  pinMode(16, OUTPUT);  // GPIO16 controls relay
  Serial.println("Relay test starting...");
  Serial.println("Relay should click every 2 seconds");
}

void loop() {
  Serial.println("Relay ON");
  digitalWrite(16, HIGH);  // Energize relay (should click)
  delay(2000);
  
  Serial.println("Relay OFF"); 
  digitalWrite(16, LOW);   // De-energize relay (should click)
  delay(2000);
}