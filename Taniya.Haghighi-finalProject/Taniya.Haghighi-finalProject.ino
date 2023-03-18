#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <DS3231.h>
#include <Wire.h>
DS3231 rtc(A4,A5); //SDA and SCL are connected to pins A4 and A5.

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//------------------variables------------------
struct user{
  long userId;//6 digit number
  int userPass;//4 digit number
  byte isManager;//1 if manager and 0 is employee
  byte isPresent;//1 if present and 0 if not present
};

boolean presentManager = false;
user users[14];//users of company
char in;//read char from serial
String str;//string read from serial
char *strings[45]; // an array of pointers to the pieces of the array after strtok()
char *ptr = NULL;//pointer for splitting text by comma
byte eepromLen=0;//length of the eeprom(filled bytes)
int currentUserInd;//current user login index
byte presentUsersCount=0;//number of the people present in company
byte favTemp=0;//favorite tempreture of manager
byte currTemp=0;//current tempreture
String weekdays[7]={"sun","mon","tue","wed","thu","fri","sat"};//days of week
byte dayHours[14];//hours set for each day to turn on the air conditioner
byte pcfOut=0;//pcf as output 
byte pcfLast=0;
byte currentDay;//the day that the program runs in it
byte todayStartHour;//start hour of air conditioner today
byte todayEndHour;//end hour of air conditioner today
byte secure=0;//byte read from pcf8574 for security of rooms
enum wc_state {wc_on,wc_on_1,wc_off};//states for turning wc fan on and off after 7 seconds
wc_state wc;//current state of wc fan
byte wcTimer=0;//global variable of wc fan timer
enum aisle_state{aisle_on,aisle_on_1,aisle_off};
aisle_state aisle;
byte aisleTimer=0;
enum rest_state{rest_on,rest_on_1,rest_off};
rest_state rest;
byte restTimer=0;
enum fan_state{fan_on,fan_on_1,fan_off};
fan_state fan;
byte fanTimer=0;
//---------------------------------------------
void readFromEEPROM();
char getKeyState();
void waitForKeyRelease();

void setup() {
  
  readFromEEPROM();
  
  Wire.begin();
  
  Serial.begin(9600);
  Serial.setTimeout(100);

  lcd.begin(20,4);
 
  rtc.begin();
  rtc.setTime(2,59,45);
  rtc.setDate(30,1,2022);
  currentDay =  weekday(2022,1,30);
  
  pinMode(10,INPUT_PULLUP);//manager security sensor
  pinMode(13,INPUT_PULLUP);//restuarnt light sensor
  pinMode(12,INPUT_PULLUP);//aisle light sensor
  pinMode(2,OUTPUT);//buzzer
  pinMode(3,OUTPUT);//aisle lamp
  pinMode(11,OUTPUT);//resuarant lamp
  if(presentUsersCount>0){
    pcfOut |= (0x02);//general light
    //pcfOut |= (0x04);//wc fan
    wc=wc_on;
  }
  if(digitalRead(12)==LOW){
    aisle=aisle_on;
  }else{
    aisle=aisle_off;
  }
  if(digitalRead(13)==LOW){
    rest=rest;
  }else{
    rest=rest_off;
  }
  
  initTimer();
}

void initTimer(){
//  TCNT1=0;
//  OCR1A=2666-1;
//  TCCR1A=0;
//  TCCR1B=0x09;
//  TIMSK1=(1<<OCIE1A);
//  interrupts();
  TCNT1 = 0;
  OCR1A = 15625 - 1; // 16M / 1024 = 15625
  TCCR1A = 0;
  TCCR1B = 0x0D; // N = 1024

  TIMSK1 = (1<<OCIE1A);
  interrupts();

}

ISR(TIMER1_COMPA_vect)
{
  security();
//   ligthsControl();
//    timerControl();
//    pompControl();
    
}

void security(){
    if(presentUsersCount==0){
//      Serial.println("reading pcf");
        interrupts();
//        Serial.println("in");
        Wire.requestFrom(0x20,1,true);
//        Serial.println("req");
        while(Wire.available()){
        secure=Wire.read();
        }
        
        //Serial.println(secure);
        String room=findRoom(secure);
       
        if(room!=""){
          Serial.println("sending sms and calling");
          Serial.println("warning from room: ");
          Serial.println(room);
          digitalWrite(2,HIGH);
        }else{
          digitalWrite(2,LOW);
        }
    }
}



void ligthsControl(){
  if((pcfOut&(0x02))==2){
    //aisle sensor control
      if(digitalRead(12)==LOW){
//        Serial.print("aisle");
        digitalWrite(3,HIGH);
        aisle=aisle_on ;
      }else if(aisle==aisle_on){
         aisle=aisle_on_1;
      }else if(aisle==aisle_on_1){
         aisleTimer++;
         if(aisleTimer==50){
          aisleTimer=0;
          digitalWrite(3,LOW);
          aisle=aisle_off;
         }
      }
 
    //restuarant sensor control
    if(digitalRead(13)==LOW){
        digitalWrite(11,HIGH);
        rest=rest_on ;
      }else if(rest==rest_on){
         rest=rest_on_1;
      }else if(rest==rest_on_1){
         restTimer++;
         if(restTimer==100){
          restTimer=0;
          digitalWrite(11,LOW);
          rest=rest_off;
         }
      }
  }
}


void timerControl(){
  if(wc==wc_on){
//    Serial.println("on");
    pcfOut |= (0x04);
  }else if(wc==wc_on_1){
    wcTimer++;
//    Serial.println(wcTimer);
    if(wcTimer==70){
//       Serial.println("off");
      wc=wc_off;
      wcTimer=0;
    }
  }else if(wc==wc_off){
//    Serial.println("off");
     pcfOut &= (0xFB);//wc
  }
}



void loop() {  
  todayStartHour=dayHours[currentDay*2];
  todayEndHour=dayHours[currentDay*2+1];
  Serial.println(currentDay);
  Serial.println(todayStartHour);
  Serial.println(todayEndHour);
  currentSituationMenu();
}

void clearEEPROM(){
  int i=0;
  for(i;i<EEPROM.length();i++){
    EEPROM.write(i,0);
  }
}

String findRoom(byte secure){
//  Serial.println(secure);
  String room="";
  if(secure==0){
    return room;
  }
  if((secure & 0x01)==0){
    room+=",sales room";
  } if((secure & 0x02)==0){
    room+=",production room";
  } if((secure & 0x04)==0){
    room+=",aisle room";
  } if((secure & 0x08)==0){
    room+=",recep room";
  } if((secure & 0x10)==0){
    room+=",wc room";
  } if((secure & 0x20)==0){
    room+=",union room";
  } if((secure & 0x40)==0){
    room+=",restuarant room";
  }else if((secure & 0x80)==0){
    room+=",repairing room";
  } if(digitalRead(10)==LOW && !presentManager){
    room+="manager room";
  } 
  return room;
}

int weekday(int year, int month, int day)
/* Calculate day of week in proleptic Gregorian calendar. Sunday == 0. */
{
  int adjustment, mm, yy;
  if (year<2000) year+=2000;
  adjustment = (14 - month) / 12;
  mm = month + 12 * adjustment - 2;
  yy = year - adjustment;
  return (day + (13 * mm - 1) / 5 +
    yy + yy / 4 - yy / 100 + yy / 400) % 7;
}

void heaterControl(){
  Time t = getRTC();
  if((t.hour<todayEndHour && t.hour>=todayStartHour) || presentUsersCount>0){
     currTemp = (long)analogRead(A1) * 500 / 1024;
      if(favTemp-currTemp>0){
         pcfOut |= (0x10);
      }else{
         pcfOut &= (0xEF);
      }
  }else{
         pcfOut &= (0xEF);
      }
}

void pompControl(){
  Time t = getRTC();
  if((t.hour<todayEndHour && t.hour>=todayStartHour) || presentUsersCount>0){
  currTemp = (long)analogRead(A1) * 500 / 1024;
    if(currTemp-favTemp>0){
      if(fan!=fan_on_1)
      fan=fan_on;
    }else if(fan!=fan_off){
      fan=fan_off;
    }
  }else{
    pcfOut &= (0xDF);//pomp
     pcfOut &= (0xBF);//motor
     pcfOut &= (0x7F);//fast
    return;
  }
  if(fan==fan_on){
      pcfOut |= (0x20);//pomp
        fanTimer++;
    if(fanTimer==40){
      pcfOut |= (0x40);//motor
      fan=fan_on_1;
    }
  } if(fan==fan_on_1){
    fanTimer=0;
     if(currTemp-favTemp>4){
       pcfOut |= (0x80);//fast
      }else{
       pcfOut &= (0x7F);//fast
      }
  }else if(fan==fan_off){
     pcfOut &= (0xDF);//pomp
     pcfOut &= (0xBF);//motor
     pcfOut &= (0x7F);//fast
  }
}

void currentSituationMenu(){
//clearEEPROM();
  
   Wire.beginTransmission(0x27);
   Wire.write(pcfOut);
   Wire.endTransmission();
  lcd.setCursor(0,0);
  lcd.print("current temp:"); 
   
  lcd.setCursor(0,2);
  lcd.print("fav temp:"); 
  lcd.setCursor(0,3);
  lcd.print(favTemp); 
 
  while(true){
     heaterControl();
      
       ligthsControl();
       timerControl();
       pompControl();
       if(pcfOut!=pcfLast){
         Wire.beginTransmission(0x27);
         Wire.write(pcfOut);
         Wire.endTransmission();
         pcfLast=pcfOut;
       }
      //manager lock
      if(presentManager){
        pcfOut |= (0x08);
      }else{
        pcfOut &= (0xF7);
      }
    readSerial();
   
  currTemp = (long)analogRead(A1) * 500 / 1024;
  lcd.setCursor(0,1);
  lcd.print(currTemp);
  if(currTemp<10){
    lcd.setCursor(1,1);
  lcd.print(" ");
  }
  
  char key = getKeyState();
  delay(100); 
  if(key!=0){
    waitForKeyRelease();
    lcd.clear();
    startMenu();
    lcd.setCursor(0,0);
  lcd.print("current temp:"); 
   
  lcd.setCursor(0,2);
  lcd.print("fav temp:"); 
  lcd.setCursor(0,3);
  lcd.print(favTemp);
  }

  }
 
}

void startMenu(){
  //  clearEEPROM();
    int idPage=0;
    int temp = 0;
   
    byte rightSlide=0;
    lcd.setCursor(0,temp);
    lcd.print('>');
    showIdPage(idPage);
    boolean isNotCanceled=true;
    while(isNotCanceled)
  {  
       ligthsControl();
       timerControl();
       pompControl();
       
       heaterControl();
       
      //manager lock
      if(presentManager){
        pcfOut |= (0x08);
      }else{
        pcfOut &= (0xF7);
      }
     
      lcd.setCursor(0,temp);
      lcd.print('>'); 
       if(pcfOut!=pcfLast){
         Wire.beginTransmission(0x27);
         Wire.write(pcfOut);
         Wire.endTransmission();
         pcfLast=pcfOut;
       }
      readSerial();
      char key = getKeyState();
      Serial.print(key);
      if(key != 0)
      {
          lcd.setCursor(0,temp);
          lcd.print(' ');      
      }
      delay(100);  
    switch(key)
    {
     
        case 'r'://right
            if(idPage<=(eepromLen/24)+3){
              lcd.clear();
              idPage+=3;
              temp = 0;
              rightSlide++;
              showIdPage(idPage);
            }
             waitForKeyRelease();
        break;
        case 'l'://left
           
           if(idPage>0){
             lcd.clear();
              idPage-=3;
              temp = 0;
              rightSlide--;
              showIdPage(idPage);
            }
             waitForKeyRelease();
        break;
      case 'u': // UP
          if(temp>0){
            temp = temp - 1;
          }
          lcd.setCursor(0,temp);
          lcd.print('>');
        
          waitForKeyRelease();
        break;

      case 'd': //DOWN
      
          if(temp<((eepromLen/8)-3*rightSlide)-1){
            if(temp<2){
             temp = temp + 1;
            }
              lcd.setCursor(0,temp);
              lcd.print('>');
              waitForKeyRelease();
          }
         
          break;

      case 'o': //OK
          lcd.clear();
          delay(100);
         enterPassword(temp,idPage);
         lcd.clear();
        showIdPage(idPage);
         lcd.setCursor(0,temp);
         lcd.print('>');        
  
          waitForKeyRelease();
        break;
        case 'c': //cancel
            isNotCanceled=false;
            lcd.clear();
        break;
    }
     
  }
  delay(100);
   
}


void enterPassword(int t,int idPage){
  int pass=users[idPage+t].userPass;
  long id=users[idPage+t].userId;
  byte isM=users[idPage+t].isManager;
  currentUserInd=idPage+t;
   lcd.clear();
   lcd.setCursor(2,0);
   lcd.print(id);
   lcd.setCursor(2,1);
   lcd.print(pass);
   int temp=2;
   lcd.setCursor(0,2);
   int digit=0;
   lcd.print(">"); 
   int enteredPass=0;
//   waitForKeyRelease();
   boolean isNotCanceled=true;
    while(isNotCanceled)
  {
    readSerial();
      char key = getKeyState();
      if(key != 0)
      {
          lcd.setCursor(temp,2);
      }
      delay(100);  
      
    switch(key)
    {
      case 'u': // UP
          lcd.setCursor(temp,2);
          if(digit<9){
            digit++;
          }else{
            digit=0;
          }
          lcd.print(digit); 
          waitForKeyRelease();
        break;

      case 'd': //DOWN
          lcd.setCursor(temp,2);
          if(digit>0){
            digit--;
          }else{
            digit=9;
          }
          lcd.print(digit); 
          waitForKeyRelease();
          break;

      case 'o': //OK
//      Serial.println(temp);
//            delay(20);
             enteredPass=enteredPass*10;
             enteredPass+=digit;
             lcd.setCursor(4,3);
             lcd.print(enteredPass);
          if(temp<5){
            temp++;
          }else if(enteredPass==pass){
             lcd.setCursor(temp,3);
             lcd.print("entered"); 
             delay(100);
             if(isM==1){
               showManagerMenue();
             }else{
                showEmployeeMenu();
             }
             isNotCanceled=false;
            }else{
              isNotCanceled=false;
            }
          lcd.setCursor(temp,2);
          digit=0;
          lcd.print(digit);

          waitForKeyRelease();
        break;
        case 'c': //cancel
        isNotCanceled=false;
        
        break;
    }
     ligthsControl();
       timerControl();
       pompControl();
  }
  delay(50);
}
void managerMenu(){
   lcd.setCursor(2,0);
      lcd.print("enter");
      lcd.setCursor(2,1);
      lcd.print("leave");
      lcd.setCursor(2,2);
      lcd.print("temp setting");
      lcd.setCursor(2,3);
      lcd.print("week days");
}

void showManagerMenue(){
    lcd.clear();
    int temp = 0;
    lcd.setCursor(0,temp);
    lcd.print('>');
   managerMenu();
    boolean isNotCanceled=true;
//     waitForKeyRelease();
    while(isNotCanceled)
  {   
      if(presentManager){
        digitalWrite(2,HIGH);
      }else{
        digitalWrite(2,LOW);
      }
      ligthsControl();
       timerControl();
       pompControl();
       readSerial();
      lcd.setCursor(0,temp);
      lcd.print('>'); 
      char key = getKeyState();
      if(key != 0)
      {
          lcd.setCursor(0,temp);
          lcd.print(' ');      
      }
      delay(100); 
       
    switch(key)
    {
      case 'u': // UP
          temp = temp - 1;
          lcd.setCursor(0,temp);
          lcd.print('>');
        
          waitForKeyRelease();
        break;

      case 'd': //DOWN
            temp = temp + 1;
            lcd.setCursor(0,temp);
            lcd.print('>');
          waitForKeyRelease();
          break;

      case 'o': //OK
      
          if(temp<2){
              changePresence(temp);
               
          }else if(temp==2){
            lcd.clear();
            changeTemp();
          
          }else{
            lcd.clear();
            weekDaysMenu();
            
          }
          waitForKeyRelease();
          lcd.clear();
           managerMenu();
         lcd.setCursor(0,temp);
         lcd.print('>');
        break;
        case 'c': //cancel
          isNotCanceled=false;
        break;
    }
//    Wire.beginTransmission(0x27);
//      Wire.write(pcfOut);
//      Wire.endTransmission();
      
  }
  delay(100);
}

void changeTemp(){
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("current:");
  lcd.setCursor(1,1);
  lcd.print("enter fav temp:");
   int temp=1;
   lcd.setCursor(0,2);
   lcd.print('>'); 
   lcd.setCursor(1,2);
   lcd.print(favTemp);
   int digit=favTemp;
   //waitForKeyRelease();
   boolean isNotCanceled=true;
    while(isNotCanceled)
  {
    readSerial();
     ligthsControl();
       timerControl();
       pompControl();
      currTemp = (long)analogRead(A1) * 500 / 1024;
      lcd.setCursor(9,0);
      lcd.print(currTemp);
      char key = getKeyState();
      
      delay(100);  
    switch(key)
    {
      case 'u': // UP
          lcd.setCursor(temp,2);
            digit++;
            if(digit==-1){
              lcd.setCursor(2,2);
              lcd.print(" ");
            }
          lcd.print(digit); 
          waitForKeyRelease();
        break;

      case 'd': //DOWN
          lcd.setCursor(temp,2);
            digit--;
            if(digit==10){
              lcd.setCursor(2,2);
              lcd.print(" ");
            }
          lcd.print(digit); 
          waitForKeyRelease();
          break;

      case 'o': //OK
          lcd.setCursor(4,3);
          lcd.print(digit);
          EEPROM.write(1, digit);
          favTemp=digit;
          lcd.setCursor(temp,2);
          lcd.print(digit);
          waitForKeyRelease();
          isNotCanceled=false;
        break;
        case 'c': //cancel
        isNotCanceled=false;
        break;
    }
     
  }
  delay(100);
}

void showEmployeeMenu(){
    lcd.clear();
    int temp = 0;
    lcd.setCursor(0,temp);
    lcd.print('>');
     //waitForKeyRelease();
    boolean isNotCanceled=true;
    while(isNotCanceled)
  {
    readSerial();
     ligthsControl();
       timerControl();
       pompControl();
        lcd.setCursor(2,0);
      lcd.print("enter");
      lcd.setCursor(2,1);
      lcd.print("leave");
      lcd.setCursor(0,temp);
      lcd.print('>'); 
      char key = getKeyState();
      if(key != 0)
      {
          lcd.setCursor(0,temp);
          lcd.print(' ');      
      }
      delay(100);  
    switch(key)
    {
      case 'u': // UP
          temp = temp - 1;
          lcd.setCursor(0,temp);
          lcd.print('>');
        
          waitForKeyRelease();
        break;

      case 'd': //DOWN
          if(temp<1){
            temp = temp + 1;
            lcd.setCursor(0,temp);
            lcd.print('>');
          }
  
          waitForKeyRelease();
          break;

      case 'o': //OK
          changePresence(temp);
          lcd.clear();
         waitForKeyRelease();
         lcd.setCursor(0,temp);
         lcd.print('>');
        break;
        case 'c': //cancel
          isNotCanceled=false;
        break;
    }
     
  }
  delay(100);
}

void changePresence(int row){
  waitForKeyRelease();
  lcd.clear();
  String currDate=getTimeFromRTC();
  if(row==0){
        EEPROM.write((currentUserInd*8)+9, 1);
        users[currentUserInd].isPresent=1;
        if(users[currentUserInd].isManager==1){
          presentManager=true;
          pcfOut |= (0x08);
        }
        if(users[currentUserInd].isPresent==1){
          presentUsersCount++;
          wc=wc_on;
          pcfOut|=(0x02);//general light
          //pcfOut|=(0x04);//wc
        }
        Serial.print("Person ");
        Serial.print(users[currentUserInd].userId);
        Serial.print(" , ");
        Serial.print(currDate);
        Serial.print(" , ");
        Serial.println("IN");
  }else{
    EEPROM.write((currentUserInd*8)+9, 0);
    users[currentUserInd].isPresent=0;
    if(users[currentUserInd].isManager==1){
      presentManager=false;
      pcfOut &= (0xF7);
    }
    if(users[currentUserInd].isPresent==0){
      presentUsersCount--;
    }
    Serial.print(presentUsersCount);
    if(presentUsersCount==0){
          pcfOut &= (0xFD);//general light
          //pcfOut &= (0xFB);//wc
          wc=wc_on_1;
     }
    Serial.print("Person ");
    Serial.print(users[currentUserInd].userId);
    Serial.print(" , ");
    Serial.print(currDate);
    Serial.print(" , ");
    Serial.println("OUT");
  }
//  Wire.beginTransmission(0x27);
//  Wire.write(pcfOut);
//  Wire.endTransmission();
}

Time getRTC(){
   Time currentTime = rtc.getTime();
}

String getTimeFromRTC(){
 Time currentTime = getRTC();

String timeString="";

// hour:min:sec
timeString.concat(currentTime.hour);
timeString.concat(":");
timeString.concat(currentTime.min);
return timeString;
}

void showIdPage(int j){
  
  int i=0;
  for(i;i<3;i++){
    if(j<14){
     
      lcd.setCursor(2,i);
      String myString = String(users[j].userId);
      String s =(users[j].userId>0)?(myString):"";
     
      lcd.print(s);
      j++;
    }
  }
    lcd.setCursor(0,3);
    lcd.print("LEFT:PREV,RIGHT:NEXT");
}

void weekDaysMenu(){
    int idPage=0;
    int temp = 0;
    lcd.setCursor(0,temp);
    lcd.print('>');
    //waitForKeyRelease();
    boolean isNotCanceled=true;
    while(isNotCanceled)
  {   
    readSerial();
    ligthsControl();
       timerControl();
       pompControl();
      lcd.setCursor(0,temp);
      lcd.print('>'); 
      showWeekDayPage(idPage);
      char key = getKeyState();
      if(key != 0)
      {
          lcd.setCursor(0,temp);
          lcd.print(' ');      
      }
      delay(100);  
    switch(key)
    {
        case 'r'://right
            if(idPage<= 2){
              lcd.clear();
              idPage+=3;
              temp = 0;
            }
             waitForKeyRelease();
        break;
        case 'l'://left
           
           if(idPage>0){
             lcd.clear();
              idPage-=3;
              temp = 0;
            }
             waitForKeyRelease();
        break;
      case 'u': // UP
          temp = temp - 1;
          lcd.setCursor(0,temp);
          lcd.print('>');
        
          waitForKeyRelease();
        break;

      case 'd': //DOWN
          temp = temp + 1;
          lcd.setCursor(0,temp);
          lcd.print('>');
          waitForKeyRelease();
          break;

      case 'o': //OK
          enterHours(idPage,temp);
          lcd.clear();
         lcd.setCursor(0,temp);
         lcd.print('>');        
  
          waitForKeyRelease();
        break;
        case 'c': //cancel
          isNotCanceled=false;
        break;
    }
     
  }
  delay(50);
}

void enterHours(byte idPage,byte t){
  lcd.clear();
  byte currentDayInd=((idPage+t)*2)+120;
//  Serial.println(currentDayInd);
  byte startHour =  EEPROM.read(currentDayInd);
  byte endHour =  EEPROM.read(currentDayInd+1);
  if(startHour==0 && endHour==0){
    endHour=24;
  }
  lcd.setCursor(0,0);
  lcd.print("from:");
  lcd.setCursor(0,2);
  lcd.print("to:");
  //waitForKeyRelease();
  
  
   int temp=1;
   int digit=startHour;
   lcd.setCursor(8,temp);
   lcd.print(startHour);
   lcd.setCursor(8,3);
   lcd.print(endHour);
   boolean isNotCanceled=true;
    while(isNotCanceled)
  {
     readSerial();
     ligthsControl();
       timerControl();
       pompControl();
      char key = getKeyState();
      delay(100);  
    switch(key)
    {
      case 'u': // UP
            if(digit<24){
               digit++;
            }
           lcd.setCursor(8,temp);
          lcd.print(digit);
          if(startHour==24){
             lcd.setCursor(9,temp);
             lcd.print(" ");
          }
          waitForKeyRelease();
        break;

      case 'd': //DOWN
          
          if(temp==3){
            if(digit>startHour){
               digit--;
            }
          }else if(temp==1){
            if(digit>0){
                digit--;
            }
          }
          lcd.setCursor(8,temp);
          lcd.print(digit); 
           if(startHour<10){
             lcd.setCursor(9,temp);
             lcd.print(" ");
          }
          
          waitForKeyRelease();
          break;

      case 'o': //OK
           
            if(temp==1){
              temp=3;
              startHour=digit;
            }else{
              isNotCanceled=false;
              endHour=digit;
              
              EEPROM.write(currentDayInd,startHour);
              EEPROM.write(currentDayInd+1,endHour);
            }
         
          lcd.setCursor(8,temp);
          lcd.print(digit);
          if(startHour<10){
          lcd.setCursor(9,temp);
          lcd.print("");
         }
          waitForKeyRelease();
        break;
        case 'c': //cancel
        isNotCanceled=false;
        break;
    }
     
  }
  delay(50);
}

void showWeekDayPage(int j){
  
  int i=0;
  for(i;i<3;i++){
    if(j<7){
      lcd.setCursor(2,i);
      String s = String(weekdays[j]);
      lcd.print(s);
      j++;
    }
  }
  if(j<6){
    lcd.setCursor(0,3);
    lcd.print("LEFT:PREV,RIGHT:NEXT");
  }else{
    lcd.setCursor(2,3);
    lcd.print(weekdays[j]);
    lcd.setCursor(8,3);
    lcd.print("LEFT:PREV");
  }
    
}


//---------------------------------------------------------------------
//read terminal 
//
//
//---------------------------------------------------------------------

void readSerial(){
   boolean isValid=true;
   byte index=0;
  if(Serial.available()){
        in  = Serial.read();
        str=str+in;
        Serial.print(in);
         if(in=='\r'){
//           Serial.print(str);
           char s [300];
           strcpy (s, str.c_str());
            index = 0;
           ptr = strtok(s, ",");  // delimiter
           byte id = 0;
           byte pass = 1;
           byte m = 2;
           while (ptr != NULL)
           {
//              Serial.print(ptr);
              strings[index] = ptr;
              index++;
              ptr = strtok(NULL, ",");
           }
        }
      }
       if(strings[0]!=0){
         storeInEEprom();
         readFromEEPROM();
      }
  }
  
    void readFromEEPROM(){
      int i=2;
      int count=0;
      eepromLen=EEPROM.read(0);
      favTemp = EEPROM.read(1);
      Serial.print("favTemp: ");
      Serial.println(favTemp);
      
      for(i;i<eepromLen;i+=8){
            user u = {EEPROMReadlong(i),readIntFromEEPROM(i+4),EEPROM.read(i+6),EEPROM.read(i+7)};
            users[count]=u;
            
            if(users[count].isManager==1 && users[count].isPresent==1){
               presentManager |=true;
            }else{
              presentManager |=false;
            }
//            Serial.println(presentManager);
            if(users[count].isPresent==1){
               presentUsersCount++;
                wc=wc_on;
            }
            Serial.println(users[count].userId);
            Serial.println(users[count].userPass);
            Serial.println(users[count].isManager);
            Serial.println(users[count].isPresent);
            count++;
      }
      i=0;
      for(i;i<14;i++){
        dayHours[i]=EEPROM.read(i+120);
      }
      if(presentUsersCount==0){
        wc=wc_off;
      }
  }

  void storeInEEprom(){
    int i=0;
    int j=1;
    while(strings[i]!=0){
      String id(strings[i]);
      
      if(j==1){
         id=id.substring(4);
         j++;
      }
     
      String pass(strings[i+1]);
      String m(strings[i+2]);
      m.trim();
      int manager=0;
      if(m.equals("M") || m.equals("m")){
        manager=1;
      }
      long uId =atol(id.c_str());
      int uPass= atoi(pass.c_str());
        eepromLen=EEPROM.read(0);
//      Serial.println(eepromLen);
//      Serial.println(uId);
//      Serial.println(manager);
        EEPROMWritelong(eepromLen+2,uId);
        writeIntIntoEEPROM(eepromLen+6, uPass);
        EEPROM.write(eepromLen+8, manager);
        EEPROM.write(eepromLen+9, 0);
        eepromLen+=8;
        EEPROM.write(0,eepromLen);
        delay(50);
        strings[i]=0;strings[i+1]=0;strings[i+2]=0;
        i+=3;
    }
    str="";
  }

  void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

long EEPROMReadlong(long address) {
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void EEPROMWritelong(int address, long value) {
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
  
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}
  
//-----------------------------------------------------------
// The function waits until the keys are released. To debouce it waits for 20ms.
//-----------------------------------------------------------
void waitForKeyRelease()
{
  do{
    while(getKeyState() != 0) //wait for keys to be released
    {}
    delay(20);
  }while(getKeyState() != 0); //repeat if the keys are not released
}

//-----------------------------------------------------------
// It returns the pressed key.
//-----------------------------------------------------------

char getKeyState()
{
  int a = analogRead(A0);
  if(a < 100)
  {
    return 'r'; //right
  }
  if(a < 250)
  {
    return 'l'; //left
  }
  if(a < 450)
  {
    return 'u'; //up    
  }
  if(a < 650)
  {
    return 'd'; //down    
  }
  if(a < 800)
  {
    return 'o'; //select  
  } 
   if(a < 950)
  {
    return 'c'; //cancel 
  } 
  return 0; //none
}
