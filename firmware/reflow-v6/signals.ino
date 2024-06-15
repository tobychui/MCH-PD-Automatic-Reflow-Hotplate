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

//Update the button press state to global variable
void updateButtonPressState(){
  int x = digitalRead(START_BTN);
  startPressed = (x == LOW);
  x = digitalRead(STOP_BTN);
  stopPressed = (x == LOW);
}

//Handle LED states in this cycle
void handleLEDBlinking(){
  //FastBlink handler, execute every 500ms
  if (cycleCounter % 5 == 0 && fastblinkRed) {
    digitalWrite(LED_PREHEAT, LOW);
    if (fastBlinkState) {
      digitalWrite(LED_REFLOW, HIGH);
    } else {
      digitalWrite(LED_REFLOW, LOW);
    }
    fastBlinkState = !fastBlinkState;
  }

  //Blink handler, execute every 2 seconds
  if (cycleCounter % 20 == 0) {
    if (blinkYellow) {
      if (blinkState) {
        digitalWrite(LED_PREHEAT, HIGH);
      } else {
        digitalWrite(LED_PREHEAT, LOW);
      }
    } else if (blinkRed) {
      if (blinkState) {
        digitalWrite(LED_REFLOW, HIGH);
      } else {
        digitalWrite(LED_REFLOW, LOW);
      }
    }
    blinkState = !blinkState;
  }
}