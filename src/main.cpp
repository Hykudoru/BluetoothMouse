#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Vector.h>
#include <MuxJoystick.h>
#include <BleKeyboard.h>
//BleKeyboard bleKeyboard;
#include <BleMouse.h>
BleMouse bleMouse;
#include <math.h>

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


//pfunc can be reasigned at runtime to change the desired procedure invoked inside the default loop function.
typedef void (*pointerFunction)(void);
pointerFunction ptrMode;
Adafruit_SSD1306 oled = Adafruit_SSD1306(128, 32, &Wire);

const int LEFT_JOYSTICK_MUX_PORT = 0;
const int RIGHT_JOYSTICK_MUX_PORT = 7;
MuxJoystick leftJoystick(LEFT_JOYSTICK_MUX_PORT, false, false);
MuxJoystick rightJoystick(RIGHT_JOYSTICK_MUX_PORT, false, false);

const float EPSILON = 0.00001;
bool EqZero(float val) 
{
  if (val < EPSILON && val > -EPSILON)
  {
    return true;
  }

  return false;
}



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

  leftJoystick.Start();
  rightJoystick.Start();
  //bleKeyboard.begin();
  bleMouse.begin();

  oled.display();
 }

int maxMouseMoveSpeed = 20;
int maxMouseScrollSpeed = 1;
bool mouseActive = true;

void loop() 
{
  oled.clearDisplay();
  oled.setCursor(0, 0);

  Vector3<float> mouse;
  Vector3<float> mouse2;
  Vector3<float> scroll;

  if (mouseActive) 
  { 
    if (bleMouse.isConnected()) {
      mouse = leftJoystick.Read();
      mouse2 = rightJoystick.Read();
      // scroll = rightJoystick.Read();
      
      if (mouse.Magnitude() > EPSILON || mouse2.Magnitude() > EPSILON)
      {
        if (mouse.Magnitude() > EPSILON && mouse2.Magnitude() > EPSILON) {
          scroll.x = constrain((mouse.x+mouse2.x), -maxMouseScrollSpeed, maxMouseScrollSpeed);
          scroll.y = constrain((mouse.y+mouse2.y), -maxMouseScrollSpeed, maxMouseScrollSpeed);
          bleMouse.move(0, 0, scroll.y, scroll.x);
        }
        else {
          mouse.x = constrain(mouse.x, -maxMouseMoveSpeed, maxMouseMoveSpeed);
          mouse.y = constrain(mouse.y, -maxMouseMoveSpeed, maxMouseMoveSpeed);
          mouse2.x = constrain(mouse2.x, -maxMouseMoveSpeed, maxMouseMoveSpeed);
          mouse2.y = constrain(mouse2.y, -maxMouseMoveSpeed, maxMouseMoveSpeed);

          bleMouse.move((mouse.x+mouse2.x), -(mouse.y+mouse2.y));
        }
      }


      if (mouse.z || mouse2.z) {
        bleMouse.click(MOUSE_LEFT);
        delay(100);
      }
      // if (bleKeyboard.isConnected())
      // {
         
      // }
    }
  }
  oled.display();
  delay(2);
}