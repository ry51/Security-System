/*
Note: parts of the keypad code were written by Md. Iqbal Hossain
https://www.tinkercad.com/things/jN6RMshVWUV-keypad

Everything else written by River Yu
TEJ 3MI
*/

// todo: set triggered state for alarm


// Include required libraries for I2C communications with the LCD and the keypad library
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// Set lcd dimensions
LiquidCrystal_I2C lcd (0x27, 16, 2); 

// Typedef long long as ll to save space later on
typedef long long ll;

// Declare state sensors (setState = yellow LED, offState = green LED) as 13 and 12
int setState = 13;
int offState = 12;
// Declare warning as pin 11
int warning = 11;

// Declare sensor and sensor on pins 14 and 15 (A0 and A1)
int sensor = 14;
int sensor2 = 15;

/*
Declare a variable "ready" to store the state of the sensor.
If ready is 0, then the alarm will not trigger no matter what
If ready is 1, then that means the user has just entered in the code and will set the state of the alarm
If ready is 2, then the alarm is armed and will trigger if motion is detected
If ready is 3, the user can set a custom passcode
If ready is 4, the user can set a custom countdown (delay before arming the alarm and when the alarm is active)
*/
int ready = 0;

// Set the current alarm state to low when initialized
int state = LOW;

// Set the current motion state to 0 (low) when initialized
int motion = 0;

// Set the numbers that the user has already inputted to 0 when the program begins
int numbers = 0;

// Declare input as an integer array of length 6. This array will keep track of the numbers the user inputted when the alarm goes off.
int input[6];

// Input for passcode switching
int newPasscode[6];


// Set the number of rows and cols in the numpad to 4
const byte ROWS = 4;
const byte COLS = 4;

// Currently I have passcode as a constant value because the program isn't finished. I will change it so the user can manually enter in a passcode later on.
ll passcode = 123456;
// Declare entered as a variable to store the user's passcode entry. Starts at 0 when initialized.
int entered = 0;

// Declare hexaKeys as a 2-dimensional array that stores the values of each key of the keypad
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Set default countdown timer to 15 seconds
int countdownTimer = 15;

// Declare the pins of the rows and columns as the pins from 2 to 9
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};

// Create the keypad as customKeypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup() {
  // Initialize lcd
  lcd.init(); 
  // Set lcd background to light
  lcd.backlight(); 
  // Clear the lcd display when initialized
  lcd.clear();
  // Begin serial communication to the monitor
  Serial.begin(9600);
  // Set warning and states as output pins
  pinMode(warning, OUTPUT);
  pinMode(offState, OUTPUT);
  pinMode(setState, OUTPUT);
  // Set sensors as input pins
  pinMode(sensor, INPUT);
  pinMode(sensor2, INPUT);
  // This part is just a testing thing, because the chromebook loads really slowly it helps to know when the program has refreshed and restarted
  Serial.println("Program initialized");
}

void loop() {
  // First, see if any keys on the keypad were pressed
  char customKey = customKeypad.getKey();
  // If a key was pressed, see if the array input is full
  if (int(customKey)) {
    if (!input[5] && int(customKey) < 65) {
      // Since ASCII values for numbers start at 48, the value of the input can be extracted by subtracting 48. Setting the input[numbers] to the inputted key will help build the passcode later on.
      input[numbers] = customKey - 48;
      // Sets the cursor to the appropriate number position on the second row
      lcd.setCursor(numbers, 1);
      // Prints out the number that the user inputted
      lcd.print(input[numbers]);
      // Clears the remainder of the second line in order to erase the "motion detected" message when re-entering the passcode to deactivate the alarm
      lcd.setCursor(numbers + 1, 1);
      lcd.print("               ");
      // Increments the number variable to get ready for the next user input
      numbers++;
    }
  }
  
  // If ready is 4 and the user has inputted 2 digits, then set a custom countdown time
  if (input[1] && ready == 4) {
    // Use place values so input[0] is the tens digit and input[1] is the ones digit
    countdownTimer = input[0]*10 + input[1];
    // Reset input array to 6 zeroes
    resetInput();
    // Set numbers to 0 to reset the position of the "pointer" to the input array
    numbers = 0;
    // Set ready back to 0
    ready = 0;
    // Delay 300ms to ensure that the user knows the countdown has been entered correctly
    delay(300);
    // Clear lcd display
    lcd.clear();
  }
  
  // Check if the input array is full
  if (input[5]) {
    // If it is, set an attempt variable to 1. This is just for later to check if the entered passcode is correct.
    bool attempt = 1;
    // Very scuffed way of adding together the user inputs. Using the pow() function in a for loop kept returning very weird results which is why I went with the messier approach.
    if (ready != 3) {
      ll enterednum = input[0]*100000 + input[1]*10000 + input[2]*1000 + input[3]*100 + input[4]*10 + input[5];
      // Check if the enterednum is equal to the passcode. If not, set attempt to 0 (which indicates a failed attempt).
      if (enterednum != passcode) attempt = 0;
      // If attempt is still 1, the passcode must have been correct
      if (attempt == 1) {
        // If the alarm was previously triggered, entering the passcode deactivates it
        if (ready == 2) {
          ready = 0;
          lcd.setCursor(0, 0);
          lcd.print("Deactivated     ");
          digitalWrite(warning, LOW);
        } 
        // Otherwise, the user must have entered the passcode to arm it so change the ready state to 1
        else {
          ready = 1;
        }
      }
      // Otherwise, the passcode must have been wrong
      else Serial.println("Incorrect passcode");
      // Reset the input array to all zeroes so that this if statement doesn't activate every iteration through the loop
      resetInput();
      // Reset numbers to 0 to reset the next passcode entry
      numbers = 0;
      // Clear the numbers from the display with a slight delay to make the user sure that they entered the 6th number correctly.
      lcd.setCursor(0, 1);
      delay(300);
      clearRow(1);
    } else {
      // In this case, ready must have been 3 and the user is going to enter a new passcode (again use place values because pow() is weird)
      ll enterednum = input[0]*100000 + input[1]*10000 + input[2]*1000 + input[3]*100 + input[4]*10 + input[5];
      passcode = enterednum;
      // Reset ready back to 0
      ready = 0;
      // Reset input array
      resetInput();
      // Reset numbers to 0 to reset the next passcode entry
      numbers = 0;
      // Clear the numbers from the display with a slight delay to make the user sure that they entered the 6th number correctly.
      lcd.setCursor(0, 1);
      delay(300);
      lcd.clear();
    }
    
  }
  
  // Once the user inputs the correct passcode, have them choose what action they want to do
  if (ready == 1) {
    if (int(customKey)) {
      // A has an ASCII value of 65. If that key is pressed, the alarm will not trigger.
      if (int(customKey) == 65) ready = 0;
      // B has an ASCII value of 66. If that key is pressed, the alarm will trigger shortly. A delay is implemented to give the user time to leave the area or else the alarm will trigger right away.
      else if (int(customKey) == 66) {
        countdown();
      // C has an ASCII value of 67. If that key is pressed, the passcode will be changed.
      } else if (int(customKey == 67)) {
        // Prompt user to change passcode
        clearRow(0);
        lcd.setCursor(0, 0);
        lcd.print("Change passcode");
        ready = 3;
      // D has an ASCII value of 68. If that key is pressed, the countdown will be changed.
      } else if (int(customKey == 68)) {
        // Prompt user to change countdown
        clearRow(0);
        lcd.setCursor(0, 0);
        lcd.print("Set countdown");
        ready = 4;
      }
    }
  }

  // Detect motion if and only if the state of the alarm is armed
  if (ready == 2) {
    // Every loop through, check if there was any motion
    motion = digitalRead(sensor) || digitalRead(sensor2);
    // If there was motion, set the warning to high (triggers led and piezo)
    if (motion == HIGH) {
      clearRow(0);
      lcd.setCursor(0, 0);
      lcd.print("Motion Detected");
      // Have the alarm blare and flash LED for a bit to try and scare off intruders. After it settles, the alarm will stay on so the loud piezo noise would hopefully attract the attention of others.
      digitalWrite(warning, HIGH);
    }
  }
  
  // set LED states
  setStates();

}

// Declare countdown function
void countdown() {
 for (int i = 0; i < countdownTimer; i++) {
    // The following code counts down from the set countdown to 1 every second
    lcd.setCursor(0, 0);
    lcd.print("Activating in");
    lcd.setCursor(14, 0);
    lcd.print(countdownTimer - i);
    if (i >= countdownTimer - 9) {
      lcd.setCursor(15, 0);
      lcd.print(" ");
    }
    delay(1000);
  }
  // When enough seconds have passed, indicate that the alarm is armed through the lcd display and then set ready to 2 which allows it to detect motion
  lcd.setCursor(0, 0);
  lcd.print("Armed and ready");
  ready = 2; 
}

// Reset input function that resets the input array
void resetInput() {
  for (int i = 0; i < 6; i++) {
    input[i] = 0;
  } 
}

// Clear row function that clears a single row of the lcd
void clearRow(int row) {
  lcd.setCursor(0, row);
  lcd.print("                "); 
}

// State set function that sets the states of the green and yellow LEDs
void setStates() {
  if (ready == 0) digitalWrite(offState, HIGH);
  else digitalWrite(offState, LOW);
  
  if (ready == 1) digitalWrite(setState, HIGH);
  else digitalWrite(setState, LOW); 
}


