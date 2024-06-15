/*
   60W PD Powered Automatic Reflow Hoteplate
   Author: tobychui

   Firmware version for v6 or above PCB
   
   Notes:
   - Temperature sensing is done using 1k 1% resistor and a 100k Thermistor
     You might need to change the code if you are using other values
   - If you are using the type-C 6p version of the reflow PCB, and after
     pluggin in the type C cable no led lights up, try flip the type C
     cable upside down

   Button Mappings (Active Low, aka not pressing = HIGH)
   Start Btn: P1.4
   Stop Btn: P1.7

   Download Settings
   CH552
   16Mhz (internal, 3.3V or 5V)
*/
#include <Serial.h>

/* Hardware Definations */
#define TEMP_PIN 11     //P1.1
#define HEATER_PIN 34   //P3.4
#define LED_PREHEAT 15  //P1.5
#define LED_REFLOW 16   //P1.6
#define START_BTN 14    //P1.4
#define STOP_BTN 17     //P1.7

/* Software Definations */
//Timing settings. Each cycle is approximately 100ms
#define HOLDSTART_TIME 30           //START Button hold down time in cycles
#define HOLDSTOP_TIME 10            //STOP Button hold down time in cycles
#define SOAKING_TIME 600            //SOAKING temperature upkeep time in cycles
#define MAX_CONTINUE_OPR_TIME 3000  //MAX_CONTINUE_OPR automatic shutdown desolder heatbed if continue operate for ~5 minutes for safty reasons
/*
   Temperature Settings

   The ADC value can be calculated using the following line equation
   t=-0.7x+250

   where X is ADC reading and t is target temp in degree C

   Note that different setup (e.g. ref resistor value / NTC resistor value) might have different equations
   You can experiment with yours and get a few data point to plot your own line
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
#define CUTOFF_TEMP_ADC 53  //250 degree C

/* Function Prototypes */
bool updateHeaterPowerState();
void updateKeyStates();
void handleLEDBlinking();
void handleReflowProcedures();
void handleDesolderProcedures();
void enterStandbyMode();
int readTemp();


/* Runtime Variables */
bool reflowing = false;       //Reflow has started
bool reflowComplete = false;  //Indicate if the stopping is done because of reflow complete
bool startPressed = false;    //Start button is pressed down
bool stopPressed = false;     //Stop button is pressed down

/*
  Reflow Stages define what is the next state the hotplate should reach
  0: Standby
  1: Preheating
  2: Preheat Holding / Soaking
  3: Reflowing
  4: Cooling down

  10: Preheating to reflow temp for desoldering
  11: Desolder Mode (Keep at REFLOW_TEMP_ADC)
*/
int reflowStages = 0;
int startCountdown = HOLDSTART_TIME;  //Hold for HOLDSTART_TIME to start to prevent accidental touches
int stopcountdown = HOLDSTOP_TIME;    //Hold for HOLDSTOP_TIME to stop reflowing
int cycleCounter = 0;                 //Record cycle time up to 10 seconds
int soakingCountdown = SOAKING_TIME;
int desolderCountdown = MAX_CONTINUE_OPR_TIME; //Prevent overheat

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
  //Set button pins to input
  pinMode(START_BTN, INPUT);
  pinMode(STOP_BTN, INPUT);

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

  USBSerial_println("Reflow Hotplate Ready!");
}


void loop() {
  /* State Check */
  updateKeyStates();

  /* Main reflowing logics */
  //Add delay to the end of each conditions so each loop cycle is approximate 100ms
  if (reflowing && reflowStages < 10) {
    //Automatic Reflow
    handleReflowProcedures();
  } else if (reflowing && reflowStages >= 10) {
    //Desolder hotplate
    handleDesolderProcedures();
  } else {
    /* Standby Mode */
    digitalWrite(HEATER_PIN, LOW);
    blinkYellow = true;
    delay(100);  //adjust this if needed to make sure reflow cycles use the same time as standby cycles
  }

  /* Blink Signal Handlers */
  handleLEDBlinking();

  /* 
    Cycle Counter 
    Counter are limited to 600 to prevent overflow.
  */

  cycleCounter++;
  if (cycleCounter > 600) {
    cycleCounter = 0;
  }
}
