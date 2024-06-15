/* reflow.ino handles reflow procedures */

void handleReflowProcedures() {
  /* Currently is in reflow stage */
  bool tempReached = updateHeaterPowerState();  //This action takes 100ms
  if (tempReached) {
    //This stage temperature reached. Move to next stage
    if (reflowStages == 1) {
      //Preheat stage completed. Enter soaking stage
      USBSerial_println("Preheat temperature reached. Soaking started");
      targetTempADC = PREHEAT_TEMP_ADC;  //Set temperature to soaking end temp
      reflowStages = 2;                  //Set reflow stage to soaking
      soakingCountdown = SOAKING_TIME;
      fastblinkRed = true;
    } else if (reflowStages == 3) {
      //Reflowing target temperature reached. Start cooldown and shut down heater
      USBSerial_println("Reflow completed. Cooling down");
      reflowStages = 4;
      blinkYellow = false;
      blinkRed = true;
      targetTempADC = COOLDOWN_TEMP_ADC;
      targetPwrPWM = 0;

      //Reflow ended. Wait until stop being press to exit this state
    }
  }

  if (reflowStages == 2) {
    //Wait for the soaking to complete and enter reflow stage
    soakingCountdown--;
    if (soakingCountdown <= 0) {
      //Soaking completed. Enter reflow stage
      USBSerial_println("Soaking time ended. Reflow started");

      //Set Reflow LED to high
      fastblinkRed = false;
      digitalWrite(LED_REFLOW, HIGH);

      //Set to reflow temperature
      targetTempADC = REFLOW_TEMP_ADC;  //Set the target temp to reflow
      targetPwrPWM = REFLOW_PWR_PWM;    //Set power rating to reflow

      //Update the reflow stage to 3
      reflowStages = 3;
    } else if (soakingCountdown % 100 == 0) {
      USBSerial_print("Soaking cycles left: ");
      USBSerial_println(soakingCountdown);
    }
  }

  delay(1);
}