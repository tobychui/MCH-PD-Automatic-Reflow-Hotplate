//When the reflow starts, preheat leds will fast blink 
//to indicate auto reflow will start in 5 seconds
void playStartingLEDBlinks(){
  digitalWrite(LED_PREHEAT, LOW);
  for(int i = 0; i < 10; i++){
    digitalWrite(LED_PREHEAT, HIGH);
    delay(200);
    digitalWrite(LED_PREHEAT, LOW);
    delay(200);
  }
}
