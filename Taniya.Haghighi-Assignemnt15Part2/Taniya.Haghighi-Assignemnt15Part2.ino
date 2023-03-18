#include <Wire.h>
byte upper=0;
void setup() {
  Wire.begin(); 
}

void loop() {
  Wire.requestFrom(0x26, 1, false);
  while(Wire.available()){
     upper=Wire.read();
  }
  upper=(upper<<4);
  Wire.beginTransmission(0x26); //Write on device 0x26
  Wire.write(upper | 15); 
  Wire.endTransmission(false);
  delay(500);
}
