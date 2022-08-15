#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Vector.h>

// template <typename T>
// void Print(Vector3<T> vec, String header = "");
byte clamp(byte &val, byte min, byte max);
int clamp(int &val, int min, int max);
float clamp(float &val, float min, float max);

void print(Vector3<float> vec, char header[]);