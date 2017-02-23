// include the library code for LCD Display
#include <LiquidCrystal.h>

#define TICK_PERIOD 50
#define AUTO_TIME_IN_SECONDS 5
#define TELEOP_TIME_IN_SECONDS 15
#define EXTRA_TIME_TO_INPUT 3
#define AUTO_TICKS AUTO_TIME_IN_SECONDS * 1000 / TICK_PERIOD
#define TELEOP_TICKS (TELEOP_TIME_IN_SECONDS + EXTRA_TIME_TO_INPUT) * 1000 / TICK_PERIOD

#define NUMBER_OF_BUTTONS 5
#define BUTTON_5 3
#define BUTTON_4 13
#define BUTTON_3 6
#define BUTTON_2 5
#define BUTTON_1 4
#define LED_1 A0
#define LED_2 A1
#define LED_3 A2
#define LED_4 A3
#define LED_5 A4
#define GO_BUTTON 0
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

uint8_t buttonArray[NUMBER_OF_BUTTONS] = [BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4, BUTTON_5];

// Reads a button
// They're set up in pull-down orientation, so a reading of LOW is the button pushed
bool buttonRead(uint8_t buttonNumber) {
  return !digitalRead(buttonNumber);
}

// Erases the LCD by writing 16 spaces over everything
void lcdErase() {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

// MM is for master machine
enum MM_states {
  MM_waitForStart,
  MM_auto,
  MM_teleop,
  MM_display
};

void MM_tick() {
  static uint32_t ticksToNextState = 0;
  static MM_states MM_currentState = MM_waitForStart;
  switch (MM_currentState) {
    case MM_waitForStart:
      if (buttonRead(GO_BUTTON)) {
        MM_currentState = MM_auto;
        lcdErase();
        lcd.setCursor(3, 0);
        lcd.print("Autonomous");
        lcd.setCursor(5, 1);
        lcd.print("Period");
        ticksToNextState = 0;
      }
      break;
    case MM_auto:
      if (ticksToNextState < AUTO_TICKS) {
        ticksToNextState++;
      }
      else if (ticksToNextState == AUTO_TICKS) {
        MM_currentState = MM_teleop;
        lcdErase();
        lcd.setCursor(4, 0);
        lcd.print("Tele-Op");
        lcd.setCursor(5, 1);
        lcd.print("Period");
        ticksToNextState = 0;
      }
      break;
    case MM_teleop:
      if (ticksToNextState < TELEOP_TICKS) {
        ticksToNextState++;
      }
      else if (ticksToNextState == TELEOP_TICKS) {
        MM_currentState = MM_display;
        lcdErase();
        lcd.setCursor(2, 0);
        lcd.print("Match over!");
        ticksToNextState = 0;
      }
      break;
    case MM_display:
      if (buttonRead(GO_BUTTON)) {
        MM_currentState = MM_waitForStart;
        lcdErase();
        lcd.setCursor(0, 0);
        lcd.print("Waiting to begin");
      }
      break;
    default:
      break;
  }

}

void setup() {
  // put your setup code here, to run once:
  pinMode(BUTTON_5, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(LED_5, OUTPUT);
  pinMode(LED_4, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_1, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Waiting to begin");
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (buttonRead(BUTTON_4)) {
    Serial.println("I\tlove\tyou");
  }
  MM_tick();
  uint8_t three = buttonRead(BUTTON_3);
  digitalWrite(LED_3, three);
  uint8_t four = buttonRead(BUTTON_4);
  digitalWrite(LED_4, four);
  uint8_t two = buttonRead(BUTTON_2);
  digitalWrite(LED_2, two);
  uint8_t one = buttonRead(BUTTON_1);
  digitalWrite(LED_1, one);
  uint8_t five = buttonRead(BUTTON_5);
  digitalWrite(LED_5, five);
  delay(TICK_PERIOD);
}
