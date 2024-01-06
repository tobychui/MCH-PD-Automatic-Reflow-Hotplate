//Read Temp read the current temperature of the hotplate
/*
 * Read Current temperature from ADC
 * The ADC is reading from a voltage divider
 * 5V --- 1k Ohm --- (ADC) --- 100k NTC --- GND
 * 
 * ADC Reading / Temperature (degree C)
 * 250 / 25 
 * 225 / 100 
 * 155 / 160 (Preheat Temp)
 * 123 / 220 (Reflow Temp)
 * 100 / 240 (Hard cutoff Temp)
 * 
 * Each reading takes around 100ms
 */
int readTemp() {
  int numReadings = 10; // Number of readings to average
  int totalSensorValue = 0;
  
  for (int i = 0; i < numReadings; ++i) {
    totalSensorValue += analogRead(TEMP_PIN);
    delay(10); // Optional: add a small delay between readings if needed
  }

  // Calculate the average sensor value
  int averageSensorValue = totalSensorValue / numReadings;
  
  return averageSensorValue;
}

//Update heater power state base on temp reading
//The hotter the plate, the smaller the ADC reading
//Return true when the temperature is within target range
bool updateHeaterPowerState(){
  int currentADC = readTemp();
  if (currentADC <= CUTOFF_TEMP_ADC){
    digitalWrite(HEATER_PIN, LOW);
    USBSerial_println("!!! OVERHEAT !!!");
    digitalWrite(LED_PREHEAT, HIGH);
    digitalWrite(LED_REFLOW, HIGH);
    return false;
  }
  if (currentADC > targetTempADC + offset){
    //Temperature too low. Turn on the heater
    analogWrite(HEATER_PIN, targetPwrPWM);
    //USBSerial_print("+ ");
    //USBSerial_println(currentADC);
    return false;
  }else if (currentADC < targetTempADC - offset){
    //Temperature too high. Turn off the heater
    analogWrite(HEATER_PIN, 0);
    //USBSerial_print("- ");
    //USBSerial_println(currentADC);
    return false;
  }else{
    //Within range. Keep the current state
    return true;
  }
}

//Update Key States handle touch button events listening and global state update
void updateKeyStates(){
  TouchKey_Process();
  uint8_t touchResult = TouchKey_Get();

  //Start button, require hold down for 3 seconds to start
  if (touchResult & (1 << 2)) {
    startCountdown--;
    if (startCountdown <= 0 && !reflowing){
      /* START REFLOW PROCESS - PREHEAT */
      USBSerial_println("!!! Reflow Started !!!");
      reflowing = true;
      startCountdown = 0; //Prevent it from going underflow
      playStartingLEDBlinks(); //Play fast blink start warning
      targetTempADC = PREHEAT_START_ADC; //Set the target temp to preheat starting temp
      targetPwrPWM = PREHEAT_PWR_PWM; //Set power rating to preheat
      reflowStages = 1; //Set the reflow stage to preheating

      //Set the blinking LED status to off
      blinkYellow = false;
      blinkRed = false;
      fastblinkRed = false;
      digitalWrite(LED_PREHEAT, HIGH); //Light up the preheat LED
      
    }
  } else {
    //Touch released
    startCountdown = HOLDSTART_TIME;
  }

  //Stop button, stop immediately
  if (touchResult & (1 << 5)) {
    reflowing= false;
    USBSerial_println("Reflow Stopped");
    //Set target temperature to room temp and shut down heater
    targetTempADC = COOLDOWN_TEMP_ADC;
    targetPwrPWM = 0;

    //Set yellow LED to slow blinkRed
    digitalWrite(LED_REFLOW, LOW);
    digitalWrite(LED_PREHEAT, LOW);
    blinkYellow = true;
    blinkRed = false;
    fastblinkRed = false;

    //Set reflow stage to standby
    reflowStages = 0;
  }
}
