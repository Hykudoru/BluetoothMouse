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
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
  const int POT_SENSOR_PIN_GPIO_39 = 39;
  const int TOUCH_PIN_GPIO_33 = 33;
  const int TOUCH_PIN_GPIO_27 = 27;
#endif

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
  // touchAttachInterrupt(TOUCH_PIN_GPIO_33, TriggerWakeSleepState, 20);
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

  lastTimeIdle = millis();

  oled.display();
 }

// ==================================
//         Keyboard Commands
// ==================================

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

void gpeditMacro()
{
  const uint8_t str_gpedit[] = "gpedit";
  //bleKeyboard.releaseAll();

  // OPEN GPEDIT: (WinKey > type "gpedit" > Enter).
  WindowsKey(); 
  delay(1000); // wait for home menu
  bleKeyboard.write(str_gpedit, sizeof(str_gpedit)); delay(100);
  EnterKey();
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
     if (mouse.z && mouse2.z)
      { 
        gpeditMacro(); //run macro
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