/* desolder.ino handles desoldering (aka just a basic heated plate) procedures */

void handleDesolderProcedures() {
  bool tempReached = updateHeaterPowerState();  //This take around 100ms
  if (reflowStages == 10 && tempReached) {
    //Temperature reached. Stop red blinking and light up red light
    fastblinkRed = false;
    digitalWrite(LED_REFLOW, HIGH);
    desolderCountdown = MAX_CONTINUE_OPR_TIME;  //Start counting down max operation time
    reflowStages = 11; //Move to consistent heating mode
  }

  if (desolderCountdown == 0) {
    enterStandbyMode();
    desolderCountdown = 0;  //Prevent underflow
  }
  desolderCountdown--;
}