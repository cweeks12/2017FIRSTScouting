// include the library code for LCD Display
#include <LiquidCrystal.h>

// These define the timing of the system
#define TICK_PERIOD 50
#define AUTO_TIME_IN_SECONDS 5
#define TELEOP_TIME_IN_SECONDS 15
#define EXTRA_TIME_TO_INPUT 3 // Necessary for the last second inputs, after the clock expires
#define AUTO_TICKS AUTO_TIME_IN_SECONDS * 1000 / TICK_PERIOD
#define TELEOP_TICKS (TELEOP_TIME_IN_SECONDS + EXTRA_TIME_TO_INPUT) * 1000 / TICK_PERIOD
#define DEBOUNCE_TICKS 2

// These can't be #defines because apparently they're redefined somewhere else
const uint8_t BUTTON_FOUR = 13;
const uint8_t BUTTON_THREE = 6;
const uint8_t BUTTON_TWO = 5;
const uint8_t BUTTON_ONE = 4;
const uint8_t BUTTON_ZERO = 3;
#define NUMBER_OF_INPUT_BUTTONS 5
#define GO_BUTTON 0 // Main start button

// LED Pin Locations
#define LED_ZERO A0
#define LED_ONE A1
#define LED_TWO A2
#define LED_THREE A3
#define LED_FOUR A4

#define LCD_RS 7
#define LCD_ENABLE 8
#define LCD_D4 9
#define LCD_D5 10
#define LCD_D6 11
#define LCD_D7 12

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
/* More Pin Designations for the LCD
   LCD R/W pin to ground
   LCD VSS pin to ground
   LCD VCC pin to 5V
   A Pin to 5V
   K Pin to Ground
   10K resistor:
   ends to +5V and ground
   wiper to LCD VO pin (pin 3)
*/

// Array that contains all of the buttons for looping through
uint8_t buttonArray[NUMBER_OF_INPUT_BUTTONS] = {BUTTON_ZERO, BUTTON_ONE, BUTTON_TWO, BUTTON_THREE, BUTTON_FOUR};
static bool autonomous = false; // Used to tell if you're in auto
static bool teleOp = false; // Or tele-op
static bool SMlocked = true; // Locks the Scoring machine
static bool DElocked = true; // Locks the data entry machine

// Locks the Scoring Machine
void SM_lock() {
  SMlocked = true;
}

// Unlocks the Scoring Machine
void SM_unlock() {
  SMlocked = false;
}

// Locks the Data Entry Machine
void DE_lock() {
  DElocked = true;
}

// Unlocks the Data Entry Machine
void DE_unlock() {
  DElocked = false;
}

// MM is for Master Machine
enum MM_states {
  MM_waitForStart, // Wait to start
  MM_dataInput, // Input the data
  MM_auto, // Autonomous mode
  MM_teleop, // Tele-operated mode
  MM_transmit, // State to transmit the data
  MM_done // Done and wait for next match
} MM_currentState; // Current state variable

// SM is for Scoring Machine
enum SM_states {
  SM_init, // Initial state
  SM_waitForButton, // Wait for button to be pushed
  SM_debounceButton, // Debounce the button
  SM_scoreAdd, // Add to the score
  SM_disabled // Locked state to not mess with anything
} SM_currentState; // Current state variable

// DE is for Data Entry
enum DE_states {
  DE_init,
  DE_teamNumberEntry, // State to enter team number
  DE_allianceEntry, // Choose red or blue
  DE_stationEntry, // Station 1, 2, or 3
  DE_disabled // Lock to not run away
} DE_currentState; // Current state variable

// Erases the LCD by writing 16 spaces over everything
void lcdErase() {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

// Prints the Autonomous Period message
void lcdAutonomousPrint() {
  lcdErase();
  lcd.setCursor(3, 0);
  lcd.print("Autonomous");
  lcd.setCursor(5, 1);
  lcd.print("Period");
}

// Prints TeleOp Period message
void lcdTeleOpPrint() {
  lcdErase();
  lcd.setCursor(4, 0);
  lcd.print("Tele-Op");
  lcd.setCursor(5, 1);
  lcd.print("Period");
}

// Reads a button
// They're set up in pull-down orientation, so a reading of LOW is the button pushed
bool buttonRead(uint8_t buttonNumber) {
  return !digitalRead(buttonNumber);
}

// Cycles through all the buttons and returns a bit pattern of the button pushes
uint8_t readAllButtons() {
  uint8_t result = 0; // Stores the bit pattern
  for (uint8_t i = 0; i < NUMBER_OF_INPUT_BUTTONS; i++) {
    result &= buttonRead(buttonArray[i]) << i;
  }
  // Returns bit pattern like this:
  // button[XXX43210]. If it's a 1, it's pressed.
  return result;
}

// Helper function to read the buttons and add the appropriate score
void addToScore(){
  
}

// Tick function for the Master Machine
void MM_tick() {
  // ------------ VARIABLES --------------
  static uint32_t ticksToNextState = 0;
  static MM_states MM_currentState = MM_waitForStart;

  // ------------ TRANSITIONS ------------
  switch (MM_currentState) {
    case MM_waitForStart: // Waiting to start
      if (buttonRead(GO_BUTTON)) { // If the go button is pushed
        MM_currentState = MM_dataInput; // Start autonomous mode
      }
      break;

    case MM_dataInput:
      if (/*DE_complete()*/ true) {
        MM_currentState = MM_auto;
        autonomous = true; // Say you're in autonomous
        lcdAutonomousPrint(); // Print and say so
        ticksToNextState = 0; // Reset counter
        SM_unlock(); // Start the Scoring Machine
      }
      break;

    case MM_auto: // Autonomous State
      if (ticksToNextState < AUTO_TICKS) { // If it's not time to move on
        ticksToNextState++; // Increment counter
      }
      else if (ticksToNextState == AUTO_TICKS) { // If it's time to move on
        MM_currentState = MM_teleop; // Move to teleop
        autonomous = false; // Switch out of autonomous mode
        teleOp = true; // And into teleop
        lcdTeleOpPrint(); // Write to the screen
        ticksToNextState = 0; // Reset counter
      }
      break;

    case MM_teleop: // Teleoperated state
      if (ticksToNextState < TELEOP_TICKS) { // If it's not time to move on
        ticksToNextState++; // Increment counter
      }
      else if (ticksToNextState == TELEOP_TICKS) { // If it is time to be done
        teleOp = false; // Not in teleOp
        MM_currentState = MM_transmit; // Move to transmit
        lcdErase(); // erase the screen
        lcd.setCursor(2, 0);
        lcd.print("Match over!"); // And print Match Over!
        ticksToNextState = 0; // Reset the counter
        SM_lock(); // Lock the Scoring Machine
      }
      break;

    case MM_transmit:
      if (/*transmit()*/buttonRead(GO_BUTTON)) {
        MM_currentState = MM_waitForStart;
        lcdErase();
        lcd.setCursor(0, 0);
        lcd.print("Waiting to begin");
      }
      break;
    case MM_done:
      MM_currentState = MM_waitForStart;
      break;

    default:
      break;
  }

}

void SM_tick() {
  // --------- Variables ----------
  static uint32_t ticksToNextState = 0;

  // ---------- STATE ACTIONS -------
  switch (SM_currentState) {
    case SM_waitForButton: // Wait for button to be pushed
      break;
    case SM_debounceButton: // Debounce the button
      ticksToNextState++;
      break;
    case SM_scoreAdd: // Add to the score
      break;
    case SM_disabled: // Locked state to not mess with anything
      break;
    default:
      break;
  }

  // -------- TRANSITIONS ---------
  switch (SM_currentState) {
    case SM_waitForButton: // Wait for button to be pushed
      if (readAllButtons()) {
        SM_currentState = SM_debounceButton;
        ticksToNextState = 0;
      }
      else if (SMlocked) {
        SM_currentState = SM_disabled;
        ticksToNextState = 0;
      }
      break;
    case SM_debounceButton: // Debounce the button
      if (ticksToNextState == DEBOUNCE_TICKS) { // If you're ready to move on
        SM_currentState = SM_scoreAdd; // Move to the score adding state
        ticksToNextState = 0; // Reset the counter
        addToScore(); // Run the add to score function to read and add to score
      }
      else if (SMlocked) {
        SM_currentState = SM_disabled;
        ticksToNextState = 0;
      }
      break;
    case SM_scoreAdd: // Add to the score
      if (!readAllButtons()) {
        SM_currentState = SM_waitForButton;
      }
      else if (SMlocked) {
        SM_currentState = SM_disabled;
        ticksToNextState = 0;
      }
      break;
    case SM_disabled: // Locked state to not mess with anything
      if (!SMlocked) { // If you're unlocked
        SM_currentState = SM_waitForButton; // Go to start state
      }
      break;
    default:
      break;
  }
}

void DE_tick(){
  
}

void setup() {
  // put your setup code here, to run once:
  pinMode(BUTTON_FOUR, INPUT_PULLUP);
  pinMode(BUTTON_THREE, INPUT_PULLUP);
  pinMode(BUTTON_TWO, INPUT_PULLUP);
  pinMode(BUTTON_ONE, INPUT_PULLUP);
  pinMode(BUTTON_ZERO, INPUT_PULLUP);
  pinMode(LED_FOUR, OUTPUT);
  pinMode(LED_THREE, OUTPUT);
  pinMode(LED_TWO, OUTPUT);
  pinMode(LED_ONE, OUTPUT);
  pinMode(LED_ZERO, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Waiting to begin");
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  MM_tick();
  SM_tick();
  DE_tick();
  uint8_t three = buttonRead(BUTTON_TWO);
  digitalWrite(LED_TWO, three);
  uint8_t four = buttonRead(BUTTON_THREE);
  digitalWrite(LED_THREE, four);
  uint8_t two = buttonRead(BUTTON_ONE);
  digitalWrite(LED_ONE, two);
  uint8_t one = buttonRead(BUTTON_ZERO);
  digitalWrite(LED_ZERO, one);
  uint8_t five = buttonRead(BUTTON_FOUR);
  digitalWrite(LED_FOUR, five);
  delay(TICK_PERIOD);
}
