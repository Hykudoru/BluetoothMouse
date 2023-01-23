#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <BleKeyboard.h>
BleKeyboard bleKeyboard;
#include <math.h>
// My libs
#include <Functions.h>
#include <Vector.h>
#include <MuxJoystick.h>

// #include <Mouse.h>
//#include <Keyboard.h>

const float EPSILON = 0.00001;
bool EqZero(float val) 
{
  if (val < EPSILON && val > -EPSILON)
  {
    return true;
  }

  return false;
}

//pfunc can be reasigned at runtime to change the desired procedure invoked inside the default loop function.
typedef void (*pointerFunction)(void);
pointerFunction ptrMode;

Adafruit_SSD1306 oled = Adafruit_SSD1306(128, 32, &Wire);
#define DEBUGGING

#if defined(ESP32)
  const int BAUD_RATE = 115200;
  const uint16_t ADC_RESOLUTION = 4095; // 0 - 4095
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
  #define BUTTON_1 27
  #define BUTTON_2 4
  #define POTENTIOMETER_1 0b100111// A3 (39)
  #define POTENTIOMETER_2 0b100100// A4 (36)
  const int POT_SENSOR_PIN_GPIO_39 = 39;
  const int TOUCH_PIN_GPIO_33 = 33;
  const int TOUCH_PIN_GPIO_27 = 27;
#endif
uint16_t rawPot1Value;
uint16_t rawPot2Value;
unsigned long* ptrPot1;
unsigned long* ptrPot2;

#if defined(__AVR_ATmega32U4__)
  const int BAUD_RATE = 9600;
  #define BUTTON_A 9
  #define BUTTON_B 6
  #define BUTTON_C 5
#endif

const int LEFT_JOYSTICK_MUX_PORT = 0;
const int RIGHT_JOYSTICK_MUX_PORT = 7;
extern int joystickCount;
MuxJoystick leftJoystick(LEFT_JOYSTICK_MUX_PORT);
MuxJoystick rightJoystick(RIGHT_JOYSTICK_MUX_PORT);
int maxMouseMoveSpeed = 10;
int maxMouseScrollSpeed = 1;
bool mouseActive = true;
bool righthanded = true;

// Max RTC Memory = 8kb
RTC_DATA_ATTR int wakeCount = 0;
RTC_DATA_ATTR int sleepCount = 0;
RTC_DATA_ATTR bool sleeping = false;
unsigned long lastTimeTouched = 0;
unsigned long lastTimeIdle = millis(); 

// =========================================================================
//                               SLEEP / AWAKE     
// =========================================================================

void Sleep()
{
  sleepCount++;
  sleeping = true;
  Serial.println("Going to sleep... zzZ");
  Serial.flush();
  esp_deep_sleep_start();
  
}

void Awake()
{
  wakeCount++;
  sleeping = false;
  //lastTimeIdle = millis();
  //lastTimeTouched = millis();
}

void TriggerWakeSleepState()
{
  Serial.println(String("Touch"));
  if (sleeping) {
    Awake();
  }
  else {
    Sleep();
  }
}

// ================================================
//         Keyboard Commands and Macros
// ================================================

void WindowsKey()
{
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press(KEY_ESC);
  delay(100);
  bleKeyboard.releaseAll();
}

const uint8_t KEY_CR = 0X0D; // \r //Fixed
const uint8_t KEY_LF = 0X0A; // \n

void EnterKey(){
  bleKeyboard.press(KEY_CR); // carriage return
  bleKeyboard.press(KEY_LF); // line feed
  delay(100);
  bleKeyboard.releaseAll();
}

void CopyCommand()
{
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press('c');
  delay(100);
  bleKeyboard.releaseAll();
}

void PasteCommand()
{
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press('v');
  delay(100);
  bleKeyboard.releaseAll();
} 

void CloseWindowForceQuit()
{
  bleKeyboard.press(KEY_LEFT_ALT);
  bleKeyboard.press(KEY_F4);
  delay(100);
  bleKeyboard.releaseAll();
}

void WindowsSearchRun(const uint8_t *str_search)
{
  //bleKeyboard.releaseAll();
  // OPEN GPEDIT: (WinKey > type "gpedit" > Enter).
  WindowsKey(); 
  delay(1000); // wait for home menu
  bleKeyboard.write(str_search, sizeof(str_search)); delay(100);
  EnterKey();
}

void OpenCMD()
{
  const uint8_t cmd[] = "cmd";
  WindowsSearchRun(cmd);
}

void GpeditMacro()
{
  oled.clearDisplay();
  oled.println(String("Running gpedit macro..."));
  oled.display();
  
  const uint8_t gpedit[] = "gpedit";
  //bleKeyboard.releaseAll();

  // OPEN GPEDIT: (WinKey > type "gpedit" > Enter).
  WindowsSearchRun(gpedit);
  delay(2500); // wait for gpedit window

  /*----------------------------------------
  > GPEDIT Window
  ------------------------------------------*/
  for (size_t i = 0; i < 4; i++)
  {
    bleKeyboard.write(KEY_DOWN_ARROW); delay(100);
  }
  bleKeyboard.releaseAll();

// ...\Administrative Templates 
//--------------------------------------------*/
  bleKeyboard.write(KEY_RIGHT_ARROW); delay(100);
  for (size_t i = 0; i < 10; i++)
  {
    bleKeyboard.write(KEY_DOWN_ARROW); delay(100);
  }
  bleKeyboard.releaseAll();

  // ...\...\Windows Components
  //---------------------------------------------*/
  bleKeyboard.write(KEY_RIGHT_ARROW); delay(100);
  for (size_t i = 0; i < 9; i++)
  {
    bleKeyboard.write(KEY_DOWN_ARROW); delay(100);
  }
 // bleKeyboard.releaseAll();

  //  ...\...\...\Bitlocker Drive Encryption 
  //---------------------------------------------*/
  // (TAB + Down arrow (8x) + Enter) "Choose driver encryption method"
  bleKeyboard.write(KEY_TAB); delay(1000); 
  bleKeyboard.releaseAll();
  for (size_t i = 0; i < 8; i++)
  {
    bleKeyboard.write(KEY_DOWN_ARROW); delay(100);
  }
  bleKeyboard.releaseAll();

  //bleKeyboard.write(KEY_RETURN);

}

void BitlockerMacro()

{
  oled.clearDisplay();
  oled.println(String("Running Bitlocker macro..."));
  oled.display();

  const uint8_t bitlocker[] = "bitlocker";
  WindowsSearchRun(bitlocker);
  delay(2500); // wait for bitlocker

  for (size_t i = 0; i < 9; i++)
  {
    bleKeyboard.write(KEY_TAB);
  }
  EnterKey();
  EnterKey();

  //To-Do...
}

void GpeditMode() 
{
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.println(String("gpedit Macro"));
  oled.display();

  if (leftJoystick.Read().z > 0 && rightJoystick.Read().z > 0)
  {
    GpeditMacro(); //run macro
  }

  delay(10);
}

void BitLockerMode() 
{
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.println(String("BitLocker Macro"));
  oled.display();
  
  if (leftJoystick.Read().z > 0 && rightJoystick.Read().z > 0)
  {
    BitlockerMacro(); //run macro
  }

  delay(10);
}

static pointerFunction modes[] = {GpeditMode, BitLockerMode};
static int modeIndex = 0;
static int prevModeIndex = 0;

void OnClickButton1() 
{
  modeIndex = clamp(--modeIndex, 0, 3);
  ptrMode = modes[modeIndex];
  Serial.println("Button Pressed");
}

void OnClickButton2() 
{
  modeIndex = clamp(++modeIndex, 0, 3);
  ptrMode = modes[modeIndex];
  Serial.println("Button 2 Pressed");
}

// ================================================================================
//                                     SETUP       
// ================================================================================
void setup() 
{ 
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);

  // =========================
  //       SETUP DISPLAY
  // =========================
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.display();//displays initial adafruit image
  oled.clearDisplay();//clears initial adafruit image
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);

  // =========================
  //         SETUP IO
  // =========================
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(BUTTON_1, INPUT_PULLDOWN);
  pinMode(BUTTON_2, INPUT_PULLDOWN);
  // RISING & FALLING are reversed if INPUT_PULLDOWN
  attachInterrupt(BUTTON_1, OnClickButton1, RISING);
  attachInterrupt(BUTTON_2, OnClickButton2, RISING);
  
  //touchAttachInterrupt(TOUCH_PIN_GPIO_33, TriggerWakeSleepState, 20);
  // esp_sleep_enable_touchpad_wakeup();
  leftJoystick.Start();
  rightJoystick.Start();
//=============================================================
// The dual joystick controller uses two sparkfun joysticks 
// oriented properly according to h and v axis marked on board.
// If using one joystick on the index finger then the joystick 
// is inverted AND then rotated. Later in the main loop the 
// x and y axis are swaped a certain way depending on if 
// left-handed or right-handed.
//=============================================================
  if (joystickCount < 2 ) {
    leftJoystick.invertH = true;
    leftJoystick.invertV = true;
  }
  //bleMouse.begin();
  bleKeyboard.begin();

  ptrMode = *modes;//GpeditMode;
  
  lastTimeIdle = millis();

  oled.display();
 }
// =======================================================================================
//                                    MAIN PROGRAM                            
// =======================================================================================
void loop() 
{
  oled.clearDisplay();
  oled.setCursor(0, 0);

  // =========================
  //       SLEEP TIMER
  // =========================
  static unsigned long WAIT_SLEEP_MILLISEC = 10UL * 1000UL;
  unsigned long elapsedTime = millis() - lastTimeIdle;
  if (elapsedTime > WAIT_SLEEP_MILLISEC) {
    //Sleep();
  } else {
    //Serial.println("Going to sleep in " + String((WAIT_SLEEP_MILLISEC - elapsedTime)/1000UL) + " seconds");
  }

  // -------------------Code below will not run during sleep mode.-------------------------
  
  // =========================
  //          SENSORS
  // =========================
  // uint16_t touch33 = touchRead(TOUCH_PIN_GPIO_33);
  // if (touch33 < 10) {
  //   Serial.println(touch33);
  //   // touchAttachInterrupt(TOUCH_PIN_GPIO_33, Awake, 10);
  //   // esp_sleep_enable_touchpad_wakeup();
  //   // Sleep();
  // }

  long pointerSpeedMultiplier = map(analogRead(POT_SENSOR_PIN_GPIO_39), 0, 4096, 1, maxMouseMoveSpeed);

  // =========================
  //          MOUSE
  // =========================
  if (mouseActive && joystickCount > 0) 
  { 
    Vector3<float> mouse = leftJoystick.Read();
    Vector3<float> mouse2 = Vector3<float>(0,0,0);
    Vector3<float> scroll = Vector3<float>(0,0,0);

    // -------DUAL JOYSTICK MOUSE---------
    if (joystickCount > 1) {
      mouse2 = rightJoystick.Read();
    }
    else // -------INDEX FINGER MOUSE---------
    {
      // Reverse x and y axis single handed index finger joystick
      Vector3<float> tmp = mouse;
      if (righthanded) {
        // RIGHT-HAND 
        mouse.x = -tmp.y;
        mouse.y = tmp.x;
      } else { 
        // LEFT-HAND
        mouse.x = tmp.y;
        mouse.y = -tmp.x;
      }
    }

    if (bleKeyboard.isConnected())
    {
        if (ptrMode) {
          (*ptrMode)();
        }
    }

    // if (bleMouse.isConnected()) 
    // {
    //   // if either joystick moved
    //   if (mouse.Magnitude() > EPSILON || mouse2.Magnitude() > EPSILON)
    //   {
    //     // MOUSE SCROLL if both moving. Scroll direction depends on the resultant vector sum of the 2D axes.
    //     if (mouse.Magnitude() > EPSILON && mouse2.Magnitude() > EPSILON) {
    //       scroll.x = constrain((mouse.x+mouse2.x), -maxMouseScrollSpeed, maxMouseScrollSpeed);
    //       scroll.y = constrain((mouse.y+mouse2.y), -maxMouseScrollSpeed, maxMouseScrollSpeed);
          
    //       bleMouse.move(0, 0, scroll.y, scroll.x);
    //     }
    //     // MOUSE MOVE
    //     else {
    //       // Sum both vectors since we know only one joystick is moving while one or the other vector is zero.
    //       mouse.x = constrain(mouse.x, -maxMouseMoveSpeed, maxMouseMoveSpeed);
    //       mouse.y = constrain(mouse.y, -maxMouseMoveSpeed, maxMouseMoveSpeed);
    //       mouse2.x = constrain(mouse2.x, -maxMouseMoveSpeed, maxMouseMoveSpeed);
    //       mouse2.y = constrain(mouse2.y, -maxMouseMoveSpeed, maxMouseMoveSpeed);

    //       bleMouse.move((mouse.x+mouse2.x), -(mouse.y+mouse2.y));
    //     }

    //     lastTimeIdle = millis();
    //   }

    //   // JOYSTICK BUTTON PRESS
    //   if (mouse.z || mouse2.z) 
    //   {
    //     bleMouse.click(MOUSE_LEFT);
    //     delay(100);
    //     lastTimeIdle = millis();
    //   }

      
    // }
  }

  oled.display();
  delay(2);
}