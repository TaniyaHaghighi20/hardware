#include <DS3231.h>
DS3231 rtc(A4,A5); //SDA and SCL are connected to pins A4 and A5.

#include <LiquidCrystal.h>
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);


void setup()
{
rtc.begin();
lcd.begin(16,4); // set the LCD's number of columns and rows
}

void loop() {
  // put your main code here, to run repeatedly:
  lcd.clear();
Time t = rtc.getTime();

// day/month/year

lcd.setCursor(2,1); 
lcd.print(t.date); 
lcd.setCursor(5,1);
lcd.print("/"); 
lcd.setCursor(6,1); 
lcd.print(t.mon); 
lcd.setCursor(9,1);
lcd.print("/"); 
lcd.setCursor(10,1); 
lcd.print(t.year); 

// hour:min:sec

lcd.setCursor(6,2);
lcd.print(t.hour);
lcd.setCursor(9,2);
lcd.print(":");
lcd.setCursor(10,2);
lcd.print(t.min);
lcd.setCursor(13,2);
lcd.print(":");
lcd.setCursor(14,2);
lcd.print(t.sec);



delay(1000);
}
