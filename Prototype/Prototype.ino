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

#include "Keypad.h"



// define the pins of the RGB LED
#define LED_R      9
#define LED_G     10
#define LED_B     11

// Keyboard Matrix Mapping
#define Row1   4
#define Row2   5
#define Row3   6
#define Row4   7
#define Row5   8
#define Col1  18 // A0
#define Col2  19 // A1
#define Col3  20 // A2
#define Col4  21 // A3
#define Col5  22 // A4

#define numOfRows 5
#define numOfCols 5

enum
{
  eBtn_NoButtonPressed = 0,
  eBtn_1,
  eBtn_2,
  eBtn_3,
  eBtn_4,
  eBtn_5,
  eBtn_6,
  eBtn_7,
  eBtn_8,
  eBtn_9,
  eBtn_0,
  eBtn_00,
  eBtn_Dot,
  eBtn_Tab,
  eBtn_Divide,
  eBtn_Multiply,
  eBtn_BackSpace,
  eBtn_Minus,
  eBtn_Plus,
  eBtn_Enter,
  eBtn_NumLock,
  eBtn_Invalid,
  eBtn_LastButton
};

enum // Enumeration for the configuration
{ 
  eFirstConfig = 0,
  eEagle_sch = eBtn_1, // Allign the first button with the configuration
  eEagle_brd,
  eEagle_lbr,
  eLTSpice,
  eKiCad_sch,
  eKiCad_pcb,
  eKiCad_lib,
  eLastValidConfig,
  eConfigMode,
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

byte colors[eLastConfig][3] =
{//   R    G    B      Configuration
   {  0,   0,   0}, // eFirstConfig
   { 64,   0,   0}, // eEagle_sch
   {  0,  64,   0}, // eEagle_brd
   { 32,  64,   0}, // eEagle_lbr
   { 50,   0,  10}, // eLTSpice
   { 64,   0,   5}, // eKiCad_sch
   {  0,  64,   5}, // eKiCad_pcb
   { 32,  64,   5}, // eKiCad_lib
   { 64,  64,  64},  // eLastValidConfig
   { 64,  64,  64}  // eConfigMode
};


char buttons[numOfRows][numOfCols] = {
  {eBtn_NumLock, eBtn_Minus, eBtn_Multiply, eBtn_Divide,    eBtn_Invalid},
  {eBtn_7,       eBtn_8,     eBtn_9,        eBtn_BackSpace, eBtn_Invalid},
  {eBtn_4,       eBtn_5,     eBtn_6,        eBtn_Plus,      eBtn_Invalid},
  {eBtn_1,       eBtn_2,     eBtn_3,        eBtn_Enter,     eBtn_Tab},
  {eBtn_0,       eBtn_00,    eBtn_Invalid,  eBtn_Dot,       eBtn_Invalid}
};

// Connect keypad ROW0, ROW1, ROW2... to these Arduino pins.
byte rowPins[numOfRows] = { Row1, Row2, Row3, Row4, Row5 };
// Connect keypad COL0, COL1, COL2... to these Arduino pins.
byte colPins[numOfCols] = { Col1, Col2, Col3, Col4, Col5 };

// Create the Keypad
Keypad kpd = Keypad( makeKeymap(buttons), rowPins, colPins, numOfRows, numOfCols );

byte configModeActive;
char selectedConfig = eFirstConfig;

// Constant string array for the LC-Display
const char *activeConfiguration[eLastConfig];

#ifdef LCD_I2C_ACTIVE
// Set the LCD I2C address
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6 , 7, 3, POSITIVE);
#endif

byte currentConfig = eEagle_sch;  // Currently active configuration
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

  // The setting of the I/O for the Keypad Matrix is done in the library

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
  delay(500);
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

  // Set the Colors for the RGB LED to the initial configuration
  analogWrite(LED_R, ~(colors[currentConfig][0]));
  analogWrite(LED_G, ~(colors[currentConfig][1]));
  analogWrite(LED_B, ~(colors[currentConfig][2]));
}

void loop()
{
  char pressedButton = kpd.getKey();

  if (configModeActive == HIGH)
  {
    selectedConfig = pressedButton;
    pressedButton = eBtn_NumLock;
  }
  else
    selectedConfig = eBtn_NoButtonPressed;

  if (pressedButton == eBtn_0)
    Serial.println("Exit ConfigMode without change");

  switch(pressedButton)
  {
    case eBtn_NoButtonPressed: // If no button was pressed then exit
      ;
    break;

    case eBtn_NumLock: // The NumLock Button will enter the configuration selection
      configModeActive = HIGH;

      // Set color White for ConfigMode
      analogWrite(LED_R, ~colors[eConfigMode][0]);
      analogWrite(LED_G, ~colors[eConfigMode][1]);
      analogWrite(LED_B, ~colors[eConfigMode][2]);

      if (selectedConfig == eBtn_NumLock)
      {
        configModeActive = LOW;
        pressedButton = eBtn_NoButtonPressed;

        #ifdef DEBUG
        Serial.println("Exit ConfigMode without change");
        #endif

        // Set the new color for the current configuration
        analogWrite(LED_R, ~colors[currentConfig][0]);
        analogWrite(LED_G, ~colors[currentConfig][1]);
        analogWrite(LED_B, ~colors[currentConfig][2]);

      }
      else if (selectedConfig != eFirstConfig && selectedConfig < eLastValidConfig)
      {
        configModeActive = LOW;

        currentConfig = selectedConfig;
        selectedConfig = eFirstConfig;

        #ifdef LCD_I2C_ACTIVE
        writeConfigToLCD(currentConfig);
        #endif

        #ifdef DEBUG
        Serial.print("Exit ConfigMode. changed configuration to: ");
        Serial.print(activeConfiguration[currentConfig]);
        Serial.print(" currentConfig = ");
        Serial.println(currentConfig);
        #endif

        // Set the new color for the current configuration
        analogWrite(LED_R, ~colors[currentConfig][0]);
        analogWrite(LED_G, ~colors[currentConfig][1]);
        analogWrite(LED_B, ~colors[currentConfig][2]);
      }
      
    break;

    case eBtn_1:
    {
      switch(currentConfig)
      {
        case eEagle_sch:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "grid;");
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
          Serial.print(pressedButton);
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
          printDebugInfo(pressedButton, currentConfig, "CTRL+R");
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

    case eBtn_2:
    {
      switch(currentConfig)
      {
        case eEagle_sch:
        case eEagle_brd:
        case eEagle_lbr:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "window fit");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("window fit");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eLTSpice:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "SPACE");
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

    case eBtn_3:
    {
      switch(currentConfig)
      {
        case eEagle_sch:
        case eEagle_brd:
        case eEagle_lbr:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "copz");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("copz");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eLTSpice:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "KEY_F6");
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

    case eBtn_4:
    {
      switch(currentConfig)
      {
        case eEagle_sch:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "net");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("net");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eEagle_brd:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "route");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("route");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eEagle_lbr:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "wire");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("wire");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eLTSpice:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "KEY_F3");
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

    case eBtn_5:
    {
      switch(currentConfig)
      {
        case eEagle_sch:
        case eEagle_brd:
        case eEagle_lbr:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "move");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("move");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eLTSpice:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "KEY_F7");
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

    case eBtn_6:
    {
      switch(currentConfig)
      {
        case eEagle_sch:
        case eEagle_brd:
        case eEagle_lbr:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "group");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("group");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eLTSpice:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "KEY_F8");
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

    case eBtn_7:
    {
      switch(currentConfig)
      {
        case eEagle_sch:
        case eEagle_lbr:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "delete");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("delete");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eEagle_brd:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "ripup");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("ripup");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eLTSpice:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "KEY_F5");
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

    case eBtn_8:
    {
      switch(currentConfig)
      {
        case eEagle_sch:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "name");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("name");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eEagle_brd:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "delete");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("delete");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eEagle_lbr:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "undo");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("undo");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eLTSpice:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "KEY_F4");
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

    case eBtn_9:
    {
      switch(currentConfig)
      {
        case eEagle_sch:
        case eEagle_brd:
        case eEagle_lbr:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "undo");
          #endif
          #ifdef KEYBOARD_ACTIVE
          Keyboard.print("undo");
          Keyboard.write(KEY_RETURN);
          #endif
        break;

        case eLTSpice:
          #ifdef DEBUG
          printDebugInfo(pressedButton, currentConfig, "KEY_F9");
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
}

/*
 *  
 */


