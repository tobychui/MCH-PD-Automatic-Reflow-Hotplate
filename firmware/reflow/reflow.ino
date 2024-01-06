/*
   60W PD Powered Automatic Reflow Hoteplate
   Author: tobychui

   Notes:
   Temperature sensing is done using 1k 1% resistor and a 10k Thermistor
   You might need to change the code if you are using other values

   Touch Button Mappings
   Start Btn: P1.4
   Stop Btn: P1.7

   Download Settings
   CH552
   16Mhz (internal, 3.3V or 5V)
*/
#include <Serial.h>
#include <TouchKey.h>

#define TEMP_PIN 11     //P1.1
#define HEATER_PIN 34   //P3.4
#define LED_PREHEAT 15  //P1.5
#define LED_REFLOW 16   //P1.6

//Timing settings. Each cycle is approximately 100ms
#define HOLDSTART_TIME 30  //START Button hold down time in cycles
#define HOLDSTOP_TIME 10   //STOP Button hold down time in cycles
#define SOAKING_TIME 600   //SOAKING temperature upkeep time in cycles

/*
 * Temperature Settings
 * 
 * The ADC value can be calculated using the following line equation
 * t=-0.7x+250
 * 
 * where X is ADC reading and t is target temp in degree C
 * 
 * Note that different setup (e.g. ref resistor value / NTC resistor value) might have different equations
 * You can experiment with yours and get a few data point to plot your own line
 */
#define COOLDOWN_TEMP_ADC 255  //Room temperature
//Preheat temp & power
#define PREHEAT_START_ADC 143  //150 degree C
#define PREHEAT_TEMP_ADC 122   //160 degree C
#define PREHEAT_PWR_PWM 155    //60% power output
//Reflow temp & power
#define REFLOW_TEMP_ADC 58  //220 degree C
#define REFLOW_PWR_PWM 255  //100% power output
//Overheat cutoff temp
#define CUTOFF_TEMP_ADC 53  //230 degree C

//Func def
bool updateHeaterPowerState();
void updateKeyStates();
int readTemp();


//Runtimes
bool reflowing = false;       //Reflow has started
bool reflowComplete = false;  //Indicate if the stopping is done because of reflow complete
/*
  Reflow Stages define what is the next state the hotplate should reach
  0: Standby
  1: Preheating
  2: Preheat Holding / Soaking
  3: Reflowing
  4: Cooling down
*/
int reflowStages = 0;
int startCountdown = HOLDSTART_TIME;  //Hold for HOLDSTART_TIME to start to prevent accidental touches
int stopcountdown = HOLDSTOP_TIME;    //Hold for HOLDSTOP_TIME to stop reflowing
int cycleCounter = 0;                 //Record cycle time up to 10 seconds
int soakingCountdown = SOAKING_TIME;

//Blink state controls
bool blinkState = false;  //If blinking this will be switching between true and false
bool blinkYellow = true;  //Slow blink yellow if enabled (Standby). Do not enable both yellow and red at the same time
bool blinkRed = false;    //Slow blink red if enable (End of Reflow). Do not enable both yellow and red at the same time
bool fastBlinkState = false;
bool fastblinkRed = false;


//Real-time temperature control targets
int targetTempADC = 255;  //Target ADC reading to reach, 255 = room temperature
int targetPwrPWM = 0;     //Target PWM cycle for heater, 255 = full power, 0 = off
int offset = 0;           //Allow +-offset for the ADC reading before reheating / cooling

void setup() {
  //Enable the touch buttons
  //Enable all 6 channels:TIN2(P1.4),TIN5(P1.7)
  TouchKey_begin((1 << 2) | (1 << 5));

  //refer to AN3891 MPR121 Baseline System for more details
  TouchKey_SetMaxHalfDelta(164);      // increase if sensor value are more noisy
  TouchKey_SetNoiseHalfDelta(8);      // If baseline need to adjust at higher rate, increase this value
  TouchKey_SetNoiseCountLimit(10);    // If baseline need to adjust faster, increase this value
  TouchKey_SetFilterDelayLimit(5);    // make overall adjustment slower
  TouchKey_SetTouchThreshold(220);    // Bigger touch pad can use a bigger value
  TouchKey_SetReleaseThreshold(150);  // Smaller than touch threshold

  //Set LED pins to output
  pinMode(LED_PREHEAT, OUTPUT);
  pinMode(LED_REFLOW, OUTPUT);
  digitalWrite(LED_PREHEAT, LOW);
  digitalWrite(LED_REFLOW, LOW);

  //Set temp sense pin to input
  pinMode(TEMP_PIN, INPUT);

  //Set mosfet control pins to output and disable it
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);
}


void loop() {
  //Check key status
  updateKeyStates();

  //Main reflowing logics
  //Add delay to the end of each conditions so each loop cycle is approximate 100ms
  if (reflowing) {
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
  } else {
    //Standby mode
    digitalWrite(HEATER_PIN, LOW);
    blinkYellow = true;
    delay(100);
  }

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

  //Cycle printout
  //USBSerial_println("cycle");
  cycleCounter++;
  if (cycleCounter > 600) {
    cycleCounter = 0;
  }
}
