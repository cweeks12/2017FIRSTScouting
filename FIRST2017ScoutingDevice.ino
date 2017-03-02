// include the library code for LCD Display
#include <LiquidCrystal.h>

// These define the timing of the system
#define TICK_PERIOD 20
#define AUTO_TIME_IN_SECONDS 3
#define TELEOP_TIME_IN_SECONDS 4
#define EXTRA_TIME_TO_INPUT 1 // Necessary for the last second inputs, after the clock expires
#define AUTO_TICKS AUTO_TIME_IN_SECONDS * 1000 / TICK_PERIOD
#define TELEOP_TICKS (TELEOP_TIME_IN_SECONDS + EXTRA_TIME_TO_INPUT) * 1000 / TICK_PERIOD
#define DEBOUNCE_TICKS 2
#define LED_TURN_ON_DELAY_TICKS 1
#define LENGTH_OF_LCD_STRING 25
#define LENGTH_OF_SERIAL_STRING 50

#define BLUE_INPUT_BUTTON 6
#define GREEN_INPUT_BUTTON 5
#define YELLOW_INPUT_BUTTON 4
#define RED_INPUT_BUTTON 3
#define WHITE_INPUT_BUTTON 2
#define NUMBER_OF_INPUT_BUTTONS 5
#define GO_BUTTON 0 // Main start button

// LED Pin Locations
#define WHITE_LED A0
#define RED_LED A1
#define YELLOW_LED A2
#define GREEN_LED A3
#define BLUE_LED A4

// Define where the LCD pins are
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
uint8_t buttonArray[NUMBER_OF_INPUT_BUTTONS] = {WHITE_INPUT_BUTTON, RED_INPUT_BUTTON, YELLOW_INPUT_BUTTON, GREEN_INPUT_BUTTON, BLUE_INPUT_BUTTON};
static bool autonomous = false; // Used to tell if you're in auto
static bool teleOp = false; // Or tele-op
static bool SM_locked = true; // Locks the Scoring machine
static bool DE_locked = true; // Locks the data entry machine

// Locks the Scoring Machine
void SM_lock() {
  SM_locked = true;
}

// Unlocks the Scoring Machine
void SM_unlock() {
  SM_locked = false;
}

// Locks the Data Entry Machine
void DE_lock() {
  DE_locked = true;
}

// Unlocks the Data Entry Machine
void DE_unlock() {
  DE_locked = false;
}

// MM is for Master Machine
enum MM_states {
  MM_waitForStart, // Wait to start
  MM_dataInput, // Input the data
  MM_auto, // Autonomous mode
  MM_teleop, // Tele-operated mode
  MM_transmit, // State to transmit the data
  MM_done // Done and wait for next match
} MM_currentState = MM_waitForStart; // Current state variable

// SM is for Scoring Machine
enum SM_states {
  SM_waitForButton, // Wait for button to be pushed
  SM_debounceButton, // Debounce the button
  SM_scoreAdd, // Add to the score
  SM_ledDelay,  // Delays a bit to let the LEDs spin up
  SM_disabled // Locked state to not mess with anything
} SM_currentState = SM_disabled; // Current state variable

// DE is for Data Entry
enum DE_states {
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

// USER FRIENDLY HELPER FUNCTIONS

// Reads a button
// They're set up in pull-down orientation, so a reading of LOW is the button pushed
bool buttonRead(uint8_t buttonNumber) {
  return !digitalRead(buttonNumber);
}

// Cycles through all the buttons and returns true if any are pressed
bool readAllButtons() {
  for (uint8_t i = 0; i < NUMBER_OF_INPUT_BUTTONS; i++) {
    if (buttonRead(buttonArray[i])) { // Short circuits the loop here if any are pushed
      return true;
    }
  }
  return false; // If none are, it returns false
}

// User friendly way to tell an LED to turn on
void turnOn(uint8_t ledNumber) {
  digitalWrite(ledNumber, HIGH);
}

// User friendly way to tell an LED to turn off
void turnOff(uint8_t ledNumber) {
  digitalWrite(ledNumber, LOW);
}

// Turns off BLUE - RED LEDs
void turnOffTopLeds() {
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

// Turns off all LEDs
void turnOffAllLeds() {
  turnOffTopLeds();
  digitalWrite(WHITE_LED, LOW);
}

// Strings to display and give to the serial handler
char displayLineOne[LENGTH_OF_LCD_STRING] = {0}; // Strings to keep the output for
char displayLineTwo[LENGTH_OF_LCD_STRING] = {0};
char serialOut[LENGTH_OF_SERIAL_STRING] = {0}; // String to hold the serial values to push to computer

// Variables to hold autonomous values
static uint16_t highGoalAutonomous;
static uint16_t lowGoalAutonomous;
static uint8_t gearAutonomous;
static uint8_t foulAutonomous;
static bool crossLineAutonomous;

// Variables to hold tele-op values
static uint16_t highGoalTeleOp;
static uint16_t lowGoalTeleOp;
static uint8_t gearTeleOp;
static uint8_t foulTeleOp;
static bool climbTeleOp;

// Resets all the scores to zero
void resetScores() {
  highGoalAutonomous = 0;
  lowGoalAutonomous = 0;
  gearAutonomous = 0;
  foulAutonomous = 0;
  crossLineAutonomous = 0;
  highGoalTeleOp = 0;
  lowGoalTeleOp = 0;
  gearTeleOp = 0;
  foulTeleOp = 0;
  climbTeleOp = 0;
}


// Helper function to read the buttons and add the appropriate score
void scoreCalculate() {
  // All the autonomous scores
  if (autonomous) {
    if (buttonRead(BLUE_INPUT_BUTTON)) { // Score in the high goal
      highGoalAutonomous++;
      turnOn(BLUE_LED);
    }
    if (buttonRead(GREEN_INPUT_BUTTON)) { // Score in the low goal
      lowGoalAutonomous++;
      turnOn(GREEN_LED);
    }
    if (buttonRead(YELLOW_INPUT_BUTTON)) { // Score a gear
      gearAutonomous++;
      turnOn(YELLOW_LED);
    }
    if (buttonRead(RED_INPUT_BUTTON)) { // Commit a foul
      foulAutonomous++;
      turnOn(RED_LED);
    }
    if (buttonRead(WHITE_INPUT_BUTTON)) { // Cross the line for points
      crossLineAutonomous = !crossLineAutonomous; // Flip the bool
      digitalWrite(WHITE_LED, crossLineAutonomous); // And make the LED show the status
    }
  }

  // All the Tele-Op scores
  // Pretty much the same thing, just different variables
  else if (teleOp) {
    if (buttonRead(BLUE_INPUT_BUTTON)) { // Score in the high goal
      highGoalTeleOp++;
      turnOn(BLUE_LED);
    }
    if (buttonRead(GREEN_INPUT_BUTTON)) { // Score in the low goal
      lowGoalTeleOp++;
      turnOn(GREEN_LED);
    }
    if (buttonRead(YELLOW_INPUT_BUTTON)) { // Score a gear
      gearTeleOp++;
      turnOn(YELLOW_LED);
    }
    if (buttonRead(RED_INPUT_BUTTON)) { // Commit a foul
      foulTeleOp++;
      turnOn(RED_LED);
    }
    if (buttonRead(WHITE_INPUT_BUTTON)) { // Cross the line for points
      climbTeleOp = !climbTeleOp; // Flip the boolean
      digitalWrite(WHITE_LED, climbTeleOp);
    }
  }
  return;
}

void generateStringToOutput() {
  // Generates the serial output
  sprintf(serialOut, "%d\t%d\t%d\t%d\t%s\t%d\t%d\t%d\t%d\t%s",   highGoalAutonomous,
          lowGoalAutonomous,
          gearAutonomous,
          foulAutonomous,
          crossLineAutonomous ? "Y" : "N",
          highGoalTeleOp,
          lowGoalTeleOp,
          gearTeleOp,
          foulTeleOp,
          climbTeleOp ? "Y" : "N");

  sprintf(displayLineOne, "A %dH %dL %dG L:%s", highGoalAutonomous, lowGoalAutonomous, gearAutonomous, crossLineAutonomous ? "Y" : "N");
  sprintf(displayLineTwo, "T %dH %dL %dG C:%s", highGoalTeleOp, lowGoalTeleOp, gearTeleOp, climbTeleOp ? "Y" : "N");
}

void DE_debug(){
   switch (DE_currentState) {
    case DE_teamNumberEntry:
      break;
    case DE_allianceEntry:
      break;
    case DE_stationEntry:
      break;
    case DE_disabled:
      break;
    default:
      break;
  }
}

// Data entry machine
void DE_tick() {

  // ------ TRANSITIONS -------
  switch (DE_currentState) {
    case DE_teamNumberEntry:
      break;
    case DE_allianceEntry:
      break;
    case DE_stationEntry:
      break;
    case DE_disabled:
      break;
    default:
      break;
  }

  // ------- STATE ACTIONS -------
   switch (DE_currentState) {
    case DE_teamNumberEntry:
      break;
    case DE_allianceEntry:
      break;
    case DE_stationEntry:
      break;
    case DE_disabled:
      break;
    default:
      break;
  }
}

// Score Machine Debug
// Prints to the serial monitor which state you're in whenever you change states
void SM_debug() {
  static SM_states SM_previousState = SM_disabled;
  if (SM_currentState != SM_previousState) {
    switch (SM_currentState) {
      case SM_waitForButton: // Wait for button to be pushed
        Serial.println("SM_waitForButton");
        break;
      case SM_debounceButton: // Debounce the button
        Serial.println("SM_debounceButton");
        break;
      case SM_scoreAdd: // Add to the score
        Serial.println("SM_scoreAdd");
        break;
      case SM_ledDelay: // Delay for the LEDs
        Serial.println("SM_ledDelay");
        break;
      case SM_disabled: // Locked state to not mess with anything
        Serial.println("SM_disabled");
        break;
    }
    SM_previousState = SM_currentState;
  }
}

// Standard Tick Function for the Scoring Machine
void SM_tick() {

  //SM_debug();
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
    case SM_ledDelay:
      ticksToNextState++;
      break;
    case SM_disabled: // Locked state to not mess with anything
      break;
    default:
      break;
  }

  // -------- TRANSITIONS ---------
  switch (SM_currentState) {
    case SM_waitForButton: // Wait for button to be pushed
      if (readAllButtons()) { // If any button is pressed
        SM_currentState = SM_debounceButton; // Debounce it
        ticksToNextState = 0;
      }
      else if (SM_locked) { // If it gets locked
        SM_currentState = SM_disabled; // You're disabled
        turnOffAllLeds();
        ticksToNextState = 0;
      }
      break;
    case SM_debounceButton: // Debounce the button
      if (ticksToNextState == DEBOUNCE_TICKS && readAllButtons()) { // If you're ready to move on and the button is still being pushed
        SM_currentState = SM_scoreAdd; // Move to the score adding state
        ticksToNextState = 0; // Reset the counter
        scoreCalculate(); // Run the add to score function to read and add to score
      }
      else if (!readAllButtons()) { // If the button is released before you're done
        ticksToNextState = 0;
        SM_currentState = SM_waitForButton; // Go back to waiting for a button press
      }
      break;
    case SM_scoreAdd: // Add to the score
      if (!readAllButtons()) { // Wait for all the buttons to be released
        SM_currentState = SM_ledDelay; // And go back to waiting
        ticksToNextState = 0;
      }
      else if (SM_locked) { // Lock
        SM_currentState = SM_disabled;
        turnOffAllLeds();
        ticksToNextState = 0;
      }
      break;
    case SM_ledDelay: // Delay for LEDs to be visible
      if (ticksToNextState == LED_TURN_ON_DELAY_TICKS) {
        turnOffTopLeds(); // Turn off the LEDs
        ticksToNextState = 0;
        SM_currentState = SM_waitForButton; // Go back to waiting
      }
      break;
    case SM_disabled: // Locked state to not mess with anything
      if (!SM_locked) { // If you're unlocked
        SM_currentState = SM_waitForButton; // Go to start state
      }
      break;
    default:
      break;
  }
}

void MM_debug() {
  static MM_states MM_previousState = MM_done;
  if (MM_currentState != MM_previousState) {
    switch (MM_currentState) {
      case MM_waitForStart:
        Serial.println("MM_waitForStart");
        break;
      case MM_dataInput:
        Serial.println("MM_dataInput");
        break;
      case MM_auto:
        Serial.println("MM_auto");
        break;
      case MM_teleop:
        Serial.println("MM_teleop");
        break;
      case MM_transmit:
        Serial.println("MM_transmit");
        break;
      case MM_done:
        Serial.println("MM_done");
        break;
      default:
        break;

    }
    MM_previousState = MM_currentState;
  }
}

// Tick function for the Master Machine
void MM_tick() {

  //MM_debug();
  // ------------ VARIABLES --------------
  static uint32_t ticksToNextState = 0;

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
        turnOffAllLeds();
      }
      break;

    case MM_teleop: // Teleoperated state
      if (ticksToNextState < TELEOP_TICKS) { // If it's not time to move on
        ticksToNextState++; // Increment counter
      }
      else if (ticksToNextState == TELEOP_TICKS) { // If it is time to be done
        teleOp = false; // Not in teleOp

        lcdErase(); // erase the screen
        //        lcd.setCursor(2, 0);
        //        lcd.print("Match over!"); // And print Match Over!

        ticksToNextState = 0; // Reset the counter
        SM_lock(); // Lock the Scoring Machine
        generateStringToOutput();
        turnOffAllLeds();
        lcd.setCursor(0, 0);
        lcd.print(displayLineOne);
        lcd.setCursor(0, 1);
        lcd.print(displayLineTwo);
        MM_currentState = MM_transmit; // Move to transmit
      }
      break;

    case MM_transmit:
      if (buttonRead(GO_BUTTON)) {
        MM_currentState = MM_done;
        lcdErase();
      }
      if (Serial.available() > 0 && Serial.read() == 'Y') {
        Serial.println(serialOut);
      }
      break;
    case MM_done:
      if (!buttonRead(GO_BUTTON)) {
        MM_currentState = MM_waitForStart;
        resetScores();
        lcd.setCursor(0, 0);
        lcd.print("Waiting to begin");
      }
      break;

    default:
      break;
  }
}

void setup() {

  pinMode(BLUE_INPUT_BUTTON, INPUT_PULLUP);
  pinMode(GREEN_INPUT_BUTTON, INPUT_PULLUP);
  pinMode(YELLOW_INPUT_BUTTON, INPUT_PULLUP);
  pinMode(RED_INPUT_BUTTON, INPUT_PULLUP);
  pinMode(WHITE_INPUT_BUTTON, INPUT_PULLUP);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Waiting to begin");
  Serial.begin(9600);
}

// Main loop
void loop() {
  // Variable containing the previous amount of
  static uint32_t previousMillis = 0;

  // Checks to see if enough milliseconds have passed
  if (millis() >= previousMillis + TICK_PERIOD) {
    previousMillis = millis();
    MM_tick();
    SM_tick();
    DE_tick();
  }
}
