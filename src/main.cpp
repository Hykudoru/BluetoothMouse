#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <BleKeyboard.h>
//BleKeyboard bleKeyboard;
#include <BleMouse.h>
BleMouse bleMouse;
#include <math.h>
// My libs
#include <Functions.h>
#include <Vector.h>
#include <MuxJoystick.h>

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
 
#endif

#if defined(__AVR_ATmega32U4__)
  const int BAUD_RATE = 9600;
  #define BUTTON_A 9
  #define BUTTON_B 6
  #define BUTTON_C 5
  #define BUTTON_D 12C 
  #define BUTTON_MOUSE_LEFT 0xA0
  #define BUTTON_MOUSE_RIGHT 0xA1
  #define BUTTON_SWAP_JOYSTICKS 0xA2
  
  #define ANALOG_PIN_3 0xA3
  #define ANALOG_PIN_4 0xA4
  #define ANALOG_PIN_5 0xA5

  #define LED_GREEN 8
#endif
const int POT_SENSOR_PIN = 39;
const int TOUCH_PIN_TOGGLE_WAKE_SLEEP = 33;
const int LEFT_JOYSTICK_MUX_PORT = 0;
const int RIGHT_JOYSTICK_MUX_PORT = 7;
extern int joystickCount;
MuxJoystick leftJoystick(LEFT_JOYSTICK_MUX_PORT, true, true);
MuxJoystick rightJoystick(RIGHT_JOYSTICK_MUX_PORT, true, true);
int maxMouseMoveSpeed = 10;
int maxMouseScrollSpeed = 1;
bool mouseActive = true;
bool righthanded = true;


// void TriggerWakeSleepState()
// {
//   esp_sleep_enable_touchpad_wakeup();
//   sleep
// }

void setup() 
{ 
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.display();//displays initial adafruit image
  oled.clearDisplay();//clears initial adafruit image
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // touchAttachInterrupt(TOUCH_PIN_TOGGLE_WAKE_SLEEP, TriggerWakeSleepState)
  leftJoystick.Start();
  rightJoystick.Start();
  bleMouse.begin();
  //bleKeyboard.begin();

  oled.display();
 }

void loop() 
{
  oled.clearDisplay();
  oled.setCursor(0, 0);

  // SLEEP TIMER
  static ulong WAIT_SLEEP_MILLISEC = 10UL * 1000UL;
  static ulong lastTimeIdle = millis();
  ulong elapsedTime = millis() - lastTimeIdle;
  if (elapsedTime > WAIT_SLEEP_MILLISEC) {
    Serial.println(("Last time idle: ")+lastTimeIdle);
    lastTimeIdle = millis();
    //Sleep();
  }

  // --------Code below will not run during sleep mode.--------

  // uint16_t touch33 = touchRead(TOUCH_PIN_TOGGLE_WAKE_SLEEP);
  // Serial.println(touch33);
  
  //long pointerSpeedMultiplier = map(analogRead(POT_SENSOR_PIN), 0, 4096, 1, maxMouseMoveSpeed);

  if (mouseActive) 
  { 
    Vector3<float> mouse = leftJoystick.Read();
    Vector3<float> mouse2 = Vector3<float>(0,0,0);
    Vector3<float> scroll = Vector3<float>(0,0,0);

    bool dualJoysticks = joystickCount > 1;
    if (dualJoysticks) {
      mouse2 = rightJoystick.Read();
    } else {
      // Reverse x and y axis single handed index finger joystick
      Vector3<float> tmp = mouse;
      if (righthanded) {
        // RIGHT-HAND INDEX FINGER
        mouse.x = -tmp.y;
        mouse.y = tmp.x;
      } else { 
        // LEFT-HAND INDEX FINGER
        mouse.x = tmp.y;
        mouse.y = -tmp.x;
      }
    }

    if (bleMouse.isConnected()) 
    {
      // if either joystick moved
      if (mouse.Magnitude() > EPSILON || mouse2.Magnitude() > EPSILON)
      {
        // MOUSE SCROLL if both moving. Scroll direction depends on the resultant vector sum of the 2D axes.
        if (mouse.Magnitude() > EPSILON && mouse2.Magnitude() > EPSILON) {
          scroll.x = constrain((mouse.x+mouse2.x), -maxMouseScrollSpeed, maxMouseScrollSpeed);
          scroll.y = constrain((mouse.y+mouse2.y), -maxMouseScrollSpeed, maxMouseScrollSpeed);
          
          bleMouse.move(0, 0, scroll.y, scroll.x);
        }
        // MOUSE MOVE
        else {
          // Sum both vectors since we know only one joystick is moving while one or the other vector is zero.
          mouse.x = constrain(mouse.x, -maxMouseMoveSpeed, maxMouseMoveSpeed);
          mouse.y = constrain(mouse.y, -maxMouseMoveSpeed, maxMouseMoveSpeed);
          mouse2.x = constrain(mouse2.x, -maxMouseMoveSpeed, maxMouseMoveSpeed);
          mouse2.y = constrain(mouse2.y, -maxMouseMoveSpeed, maxMouseMoveSpeed);

          bleMouse.move((mouse.x+mouse2.x), -(mouse.y+mouse2.y));
        }

        lastTimeIdle = millis();
      }

      // JOYSTICK BUTTON PRESS
      if (mouse.z || mouse2.z) 
      {
        bleMouse.click(MOUSE_LEFT);
        delay(100);

        lastTimeIdle = millis();
      }
    }
  }

  oled.display();
  delay(1);
}