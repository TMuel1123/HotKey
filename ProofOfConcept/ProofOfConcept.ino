/*
 * #################
 * ###  Defines  ###
 * #################
 */
 
#define DEBUG // Uncomment for debug through the serial connection on Pin1 and Pin2
#define KEYBOARD_ACTIVE // Uncomment to activate the keyboard output
#define LCD_I2C_ACTIVE // Uncomment to activate the LCD Output

#define Serial Serial1

#define semicolon 0x3c // German keyboard layout (is "<" on US keyboard)
//#define semicolon 0x3b // US Keyboard

#ifdef LCD_I2C_ACTIVE
#include <LiquidCrystal_I2C.h>
#include <Wire.h> 
#endif

#ifdef KEYBOARD_ACTIVE
#include "Keyboard.h"
#endif

// define the pins of the RGB LED
#define LED_R     10
#define LED_G     11
#define LED_B     13

// Button Mapping
#define Button1  12
#define Button2  9
#define Button3  8
#define Button4  7
#define Button5  6
#define Button6  5
#define Button7  4
#define Button8  3
#define Button9  2

#define BtnSelectUp   18 // A0
#define BtnSelectDown 19 // A1

enum // Enumeration for the configuration
{ 
  eEagle_sch = 0,
  eEagle_brd,
  eEagle_lbr,
  eLTSpice,
  eKiCad_sch,
  eKiCad_pcb,
  eKiCad_lib,
  eLastConfig
};

enum // Enumeration for the units of the grid
{ 
  eUnit_mil = 0,
  eUnit_mm,
  eLastunit
};

/*
 * ######################################
 * ###  Global variable declarations  ###
 * ######################################
 */

int colors[eLastConfig][3] =
{//   R    G    B      Configuration
   { 64,   0,   0}, // eEagle_sch
   {  0,  64,   0}, // eEagle_brd
   { 32,  64,   0}, // eEagle_lbr
   { 50,   0,  10}, // eLTSpice
   { 64,   0,   5}, // eKiCad_sch
   {  0,  64,   5}, // eKiCad_pcb
   { 32,  64,   5}  // eKiCad_lib
};

// Constant string array for the LC-Display
const char *activeConfiguration[eLastConfig];

#ifdef LCD_I2C_ACTIVE
// Set the LCD I2C address
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6 , 7, 3, POSITIVE);
#endif

byte previousButtonState[10];   // Array to store the previous values of the push buttons
byte previousSelectButtonUpState = 1;
byte previousSelectButtonDownState = 1;

byte currentConfig = 0;  // Currently active configuration
byte configChanged = 0;  // Trigger for the LC-Display that the configuration has changed

byte  currenGridUnit  = eUnit_mil;
float currentGridSize = 0;

/*
 * ###################
 * ###  Functions  ###
 * ###################
 */

#ifdef DEBUG
void printDebugInfo(byte btn, byte cfg, const char *cmd)
{
  Serial.print("Button: ");
  Serial.print(btn);
  Serial.print(" Config: ");
  Serial.print(activeConfiguration[cfg]);
  Serial.print(" Command: ");
  Serial.println(cmd);
}
#endif

void checkCurrentConfig()
/*
 * This function reads the state of the select buttons
 * and according to their state they will be in- or decreased.
 */
{
  byte btnSelectUpState;   // 
  byte btnSelectDownState; // 
  byte setColor = LOW;

  // Monitor the select buttons
  btnSelectUpState   = digitalRead(BtnSelectUp);
  btnSelectDownState = digitalRead(BtnSelectDown);

  if ((btnSelectUpState != previousSelectButtonUpState) && (btnSelectUpState == LOW))
  {
    currentConfig = rollVar(currentConfig, 0, eLastConfig-1, HIGH);
    setColor = HIGH;

    #ifdef LCD_I2C_ACTIVE
    writeConfigToLCD(currentConfig);
    #endif

    #ifdef DEBUG
    Serial.print("Select Button Up   ");
    Serial.print(activeConfiguration[currentConfig]);
    Serial.print(" currentConfig = ");
    Serial.println(currentConfig);
    #endif
  }
  else if ((btnSelectDownState != previousSelectButtonDownState) && (btnSelectDownState == LOW))
  {
    currentConfig = rollVar(currentConfig, 0, eLastConfig-1, LOW);
    setColor = HIGH;

    #ifdef LCD_I2C_ACTIVE
    writeConfigToLCD(currentConfig);
    #endif

    #ifdef DEBUG
    Serial.print("Select Button Down ");
    Serial.print(activeConfiguration[currentConfig]);
    Serial.print(" currentConfig = ");
    Serial.println(currentConfig);
    #endif
  }
  previousSelectButtonUpState   = btnSelectUpState;
  previousSelectButtonDownState = btnSelectDownState;

  if(setColor)
  {
    setColor = LOW;
    analogWrite(LED_R, ~colors[currentConfig][0]);
    analogWrite(LED_G, ~colors[currentConfig][1]);
    analogWrite(LED_B, ~colors[currentConfig][2]);
  }

  delay(100);
}

#ifdef LCD_I2C_ACTIVE
byte writeConfigToLCD (byte currentConfig)
/*
 * This function prints the currently active configuration on the LC-Display
 */
{
  lcd.home();
  //lcd.setCursor(0,0);
  lcd.print(activeConfiguration[currentConfig]);
}
#endif

byte rollVar (byte inValue, byte minValue, byte maxValue, byte incValue)
/*
 * This function increases/decreases a variable round robin between a min and max value
 * 
 * inValue:  Value which should be increased or decreased depending on incValue
 * minValue: Minimum value. If after in/decrease of inValue the value is smaller than minValue it will be set to maxValue
 * maxValue: Maximum value. If after in/decrease of inValue the value is bigger than maxValue it will be set to minValue
 * incValue: If incValue is TRUE then the inValue will be increased
 */
{
  
  if (incValue)
    inValue++;
  else if ((inValue == 0) && (minValue == 0) && (incValue == 0))
    inValue = maxValue;
  else
    inValue--;

  if (inValue > maxValue)
    inValue = minValue;

  if (inValue < minValue)
    inValue = maxValue;

  return inValue;
}

float gridSwitcher (float inValue, byte unit)
{
  switch(unit)
  {
    case eUnit_mil:
    {
      inValue = inValue * 0.5;
      if(inValue < 6.25)
      {
        inValue = 50;
      }
    }
    break;

    default:
      #ifdef DEBUG
      Serial.println("SOMETHING WENT WRONG! Grid unit invalid!");
      #endif
//      #ifdef KEYBOARD_ACTIVE
//      Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//      #endif
      ;
  }

  return inValue;
}

void setup()
{                                 // 1234567890123456
  activeConfiguration[eEagle_sch] = "EAGLE SCH       ";
  activeConfiguration[eEagle_brd] = "EAGLE BRD       ";
  activeConfiguration[eEagle_lbr] = "EAGLE LBR       ";
  activeConfiguration[eLTSpice]   = "LTSpice         ";
  activeConfiguration[eKiCad_sch] = "KiCad SCH       ";
  activeConfiguration[eKiCad_pcb] = "KiCad PCB       ";
  activeConfiguration[eKiCad_lib] = "KiCad LIB       ";

  // Set all pins of the buttons to INPUT with activated PULLUP
  pinMode(Button1, INPUT_PULLUP);
  pinMode(Button2, INPUT_PULLUP);
  pinMode(Button3, INPUT_PULLUP);
  pinMode(Button4, INPUT_PULLUP);
  pinMode(Button5, INPUT_PULLUP);
  pinMode(Button6, INPUT_PULLUP);
  pinMode(Button7, INPUT_PULLUP);
  pinMode(Button8, INPUT_PULLUP);
  pinMode(Button9, INPUT_PULLUP);

  // Set the pins of the select buttons
  pinMode(BtnSelectUp,   INPUT_PULLUP);
  pinMode(BtnSelectDown, INPUT_PULLUP);

  // Set the pins for the RGB LED to OUTPUT
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);

  // If debug is active then initialize the serial connection
  #ifdef DEBUG
  Serial.begin(57600);
  Serial.println("DEBUG ACTIVE!");
  Serial.println(activeConfiguration[currentConfig]); // Print the currently active configuration on the LC-Display
  #endif

  #ifdef LCD_I2C_ACTIVE
  // Initialize a 16x2 LCD on the I2C Port
  lcd.begin(16,2);               // initialize the lcd 
  lcd.home();                    // go home
  delay(1000);
  lcd.setBacklight(BACKLIGHT_ON);
  lcd.clear();
  writeConfigToLCD(currentConfig);
  #ifdef DEBUG
  lcd.setCursor(0,1);        // go to the next line
  lcd.print("!!DEBUG ACTIVE!!");
  #endif
  #endif

  // If debug is not active enable the Keayboard emulation
  #ifdef KEYBOARD_ACTIVE
  // initialize control over the keyboard:
  Keyboard.begin();
  #endif

  // Initialize the array with 1
  for (byte i = 0; i <= sizeof(previousButtonState)-1; i++)
    previousButtonState[i] = HIGH;

  // Set the Colors for the RGB LED to the initial configuration
  analogWrite(LED_R, ~(colors[currentConfig][0]));
  analogWrite(LED_G, ~(colors[currentConfig][1]));
  analogWrite(LED_B, ~(colors[currentConfig][2]));
}

void loop()
{
  byte buttonState[10]; // Define the array for the buttons

  checkCurrentConfig();

  // Read all buttons
  buttonState[1] = digitalRead(Button1);
  buttonState[2] = digitalRead(Button2);
  buttonState[3] = digitalRead(Button3);
  buttonState[4] = digitalRead(Button4);
  buttonState[5] = digitalRead(Button5);
  buttonState[6] = digitalRead(Button6);
  buttonState[7] = digitalRead(Button7);
  buttonState[8] = digitalRead(Button8);
  buttonState[9] = digitalRead(Button9);

  // Iterate through all buttons and evaluate if they have been pushed
  for (byte i = 1; i < sizeof(buttonState); i++)
  {
    // If the button has been pressed
    if ((buttonState[i] != previousButtonState[i]) && (buttonState[i] == LOW))
    {
      switch(i)
      {
        case 1:
        {
          switch(currentConfig)
          {
            case eEagle_sch:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "grid;");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("grid");
              Keyboard.write(semicolon);
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eEagle_brd:
            
              currentGridSize = gridSwitcher(currentGridSize, eUnit_mil);
              #ifdef DEBUG
              Serial.print("Button: ");
              Serial.print(i);
              Serial.print(" Config: ");
              Serial.print(activeConfiguration[eEagle_brd]);
              Serial.print(" Command: grid on ");
              switch(currenGridUnit)
              {
                case eUnit_mil:
                  Serial.print("mil ");
                break;
              }
              Serial.println(currentGridSize);
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("grid on ");
              switch(currenGridUnit)
              {
                case eUnit_mil:
                  Keyboard.print("mil ");
                break;
              }
              Keyboard.print(currentGridSize);
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eLTSpice:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "CTRL+R");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.press(KEY_LEFT_CTRL);
              Keyboard.press('r');
              delay(20);
              Keyboard.releaseAll();
              #endif
            break;

            default:
              #ifdef DEBUG
              Serial.println("SOMETHING WENT WRONG! INVALID CONFIG");
              #endif
//              #ifdef KEYBOARD_ACTIVE
//              Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//              #endif
              ;
          }
        }
        break;

        case 2:
        {
          switch(currentConfig)
          {
            case eEagle_sch:
            case eEagle_brd:
            case eEagle_lbr:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "window fit");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("window fit");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eLTSpice:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "SPACE");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.write(0x20);
              #endif
            break;

            default:
              #ifdef DEBUG
              Serial.println("SOMETHING WENT WRONG! INVALID CONFIG");
              #endif
//              #ifdef KEYBOARD_ACTIVE
//              Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//              #endif
              ;
          }
        }
        break;

        case 3:
        {
          switch(currentConfig)
          {
            case eEagle_sch:
            case eEagle_brd:
            case eEagle_lbr:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "copz");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("copz");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eLTSpice:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "KEY_F6");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.write(KEY_F6);
              #endif
            break;

            default:
              #ifdef DEBUG
              Serial.println("SOMETHING WENT WRONG! INVALID CONFIG");
              #endif
//              #ifdef KEYBOARD_ACTIVE
//              Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//              #endif
              ;
          }
        }
        break;

        case 4:
        {
          switch(currentConfig)
          {
            case eEagle_sch:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "net");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("net");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eEagle_brd:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "route");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("route");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eEagle_lbr:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "wire");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("wire");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eLTSpice:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "KEY_F3");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.write(KEY_F3);
              #endif
            break;

            default:
              #ifdef DEBUG
              Serial.println("SOMETHING WENT WRONG! INVALID CONFIG");
              #endif
//              #ifdef KEYBOARD_ACTIVE
//              Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//              #endif
              ;
          }
        }
        break;

        case 5:
        {
          switch(currentConfig)
          {
            case eEagle_sch:
            case eEagle_brd:
            case eEagle_lbr:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "move");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("move");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eLTSpice:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "KEY_F7");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.write(KEY_F7);
              #endif
            break;

            default:
              #ifdef DEBUG
              Serial.println("SOMETHING WENT WRONG! INVALID CONFIG");
              #endif
//              #ifdef KEYBOARD_ACTIVE
//              Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//              #endif
              ;
          }
        }
        break;

        case 6:
        {
          switch(currentConfig)
          {
            case eEagle_sch:
            case eEagle_brd:
            case eEagle_lbr:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "group");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("group");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eLTSpice:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "KEY_F8");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.write(KEY_F8);
              #endif
            break;

            default:
              #ifdef DEBUG
              Serial.println("SOMETHING WENT WRONG! INVALID CONFIG");
              #endif
//              #ifdef KEYBOARD_ACTIVE
//              Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//              #endif
              ;
          }
        }
        break;

        case 7:
        {
          switch(currentConfig)
          {
            case eEagle_sch:
            case eEagle_lbr:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "delete");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("delete");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eEagle_brd:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "ripup");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("ripup");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eLTSpice:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "KEY_F5");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.write(KEY_F5);
              #endif
            break;

            default:
              #ifdef DEBUG
              Serial.println("SOMETHING WENT WRONG! INVALID CONFIG");
              #endif
//              #ifdef KEYBOARD_ACTIVE
//              Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//              #endif
              ;
          }
        }
        break;

        case 8:
        {
          switch(currentConfig)
          {
            case eEagle_sch:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "name");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("name");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eEagle_brd:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "delete");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("delete");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eEagle_lbr:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "undo");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("undo");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eLTSpice:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "KEY_F4");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.write(KEY_F4);
              #endif
            break;

            default:
              #ifdef DEBUG
              Serial.println("SOMETHING WENT WRONG! INVALID CONFIG");
              #endif
//              #ifdef KEYBOARD_ACTIVE
//              Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//              #endif
              ;
          }
        }
        break;

        case 9:
        {
          switch(currentConfig)
          {
            case eEagle_sch:
            case eEagle_brd:
            case eEagle_lbr:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "undo");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.print("undo");
              Keyboard.write(KEY_RETURN);
              #endif
            break;

            case eLTSpice:
              #ifdef DEBUG
              printDebugInfo(i, currentConfig, "KEY_F9");
              #endif
              #ifdef KEYBOARD_ACTIVE
              Keyboard.write(KEY_F9);
              #endif
            break;

            default:
              #ifdef DEBUG
              Serial.println("SOMETHING WENT WRONG! INVALID CONFIG");
              #endif
//              #ifdef KEYBOARD_ACTIVE
//              Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//              #endif
              ;
          }
        }
        break;

        default:
              #ifdef DEBUG
              Serial.println("SOMETHING WENT WRONG! INVALID CONFIG");
              #endif
//              #ifdef KEYBOARD_ACTIVE
//              Keyboard.print("SOMETHING WENT WRONG! INVALID CONFIG");
//              #endif
        break;
      }
      
      delay(100);
    }
    previousButtonState[i] = buttonState[i];
  }
}

/*
 *  
 */


