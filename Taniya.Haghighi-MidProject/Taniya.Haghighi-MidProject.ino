#include <LiquidCrystal.h>
#include <EEPROM.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

signed int minTemp;
signed int maxTemp;
signed int favTemp;
long currTemp;
unsigned long nextTime = 0;
int countLogs=0;
boolean isMainMenu = true;
String in;
String str;
//struct to save logs
struct tempLog{
    int current;
    int favorite;
          };

tempLog logs[8]; 

void setup() {
//to initialize eeprom for the first time
//       for (int i = 0 ; i < EEPROM.length() ; i++) {
//      EEPROM.write(i, 0);
//   }
//initializing
  minTemp=0;
  maxTemp=0;
  favTemp=0;
  readFromEEPROM();
  Serial.begin(9600);
  Serial.setTimeout(100);
  lcd.begin(20, 4);
  pinMode(11,OUTPUT);
  pinMode(12,OUTPUT);
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(10,OUTPUT);
}

void loop() {
  isMainMenu = true;
  currTemp = (long)analogRead(A1) * 500 / 1024;
  mainMenu();
}



 //---------------------------------------------------------------------
//this method prints scrolled menu
//---------------------------------------------------------------------
void showScrolledMenu(){
  isMainMenu=false;
  lcd.clear();
  
  lcd.setCursor(2,0);
  lcd.print("min Temp: ");
  lcd.setCursor(12,0);
  lcd.print(minTemp);

  if(favTemp<minTemp || favTemp>maxTemp){
    favTemp=(minTemp+maxTemp)/2;
  }

  lcd.setCursor(2,1);
  lcd.print("fav Temp: ");
  lcd.setCursor(12,1);
  lcd.print(favTemp);

  currTemp = (long)analogRead(A1) * 500 / 1024;

  lcd.setCursor(2,2);
  lcd.print("curr Temp: ");
  lcd.setCursor(12,2);
  lcd.print(currTemp);

  lcd.setCursor(2,3);
  lcd.print("log");
}


 //---------------------------------------------------------------------
//this method prints main menu
//---------------------------------------------------------------------
void showMainMenu()
{
  isMainMenu=true;
  lcd.clear();

  lcd.setCursor(2,0);
  lcd.print("max Temp: ");  
  lcd.setCursor(12,0);
  lcd.print(maxTemp);
  
  lcd.setCursor(2,1);
  lcd.print("min Temp: ");
  lcd.setCursor(12,1);
  lcd.print(minTemp);

  if(favTemp<minTemp || favTemp>maxTemp){
    favTemp=(minTemp+maxTemp)/2;
  }

  lcd.setCursor(2,2);
  lcd.print("fav Temp: ");
  lcd.setCursor(12,2);
  lcd.print(favTemp);

  currTemp = (long)analogRead(A1) * 500 / 1024;

  lcd.setCursor(2,3);
  lcd.print("curr Temp: ");
  lcd.setCursor(12,3);
  lcd.print(currTemp);
}

 //---------------------------------------------------------------------
//this method is the main method which checks serial,updates manu, controlls heaters and coolers and ...
//---------------------------------------------------------------------
void mainMenu()
{
  int temp = 0;

  showMainMenu();
  
  lcd.setCursor(0,temp);
  lcd.print('>');
  while(true)
  {
    
      saveLogs();

      readSerial();
      
      lcd.setCursor(0,temp);
      lcd.print('>'); 
      currTemp = (long)analogRead(A1) * 500 / 1024;


//updating menu
   updateMenu();
    
   
    FansControl();
    heatersControl();

    buzzerControl();

    char key = getKeyState();
    if(key != 0)
    {
        lcd.setCursor(0,temp);
        lcd.print(' ');      
    }
    delay(20);  
    switch(key)
    {
       case 'r'://right

        break;
        case 'l'://left

        break;
      case 'u': // UP
        if(temp==4){
          temp=temp-2;
        }else if(temp > 0 && isMainMenu){
          temp = temp - 1;
          }else if(temp >= 0 && !isMainMenu){
            temp = temp - 1;
          }
          
         if(!isMainMenu && temp==-1){
          showMainMenu();
          temp=0;
         }
          lcd.setCursor(0,temp);
          lcd.print('>');
        
        waitForKeyRelease();
        break;

      case 'd': //DOWN

        if(temp < 4)
        {
          temp = temp + 1;
        }
        if(temp==4 && isMainMenu){
          showScrolledMenu();
        }
        
        lcd.setCursor(0,temp);
        lcd.print('>');

        waitForKeyRelease();
        break;

      case 'o': //OK
        
        digitalWrite(12,LOW);
        digitalWrite(11,LOW);
        digitalWrite(2,LOW);
        digitalWrite(3,LOW);

        if(isMainMenu){//for main menu
           switch(temp)
        {
          case 0:
             maxTemp=changeTemp(temp,maxTemp,1);
          break;
          case 1: 
            minTemp=changeTemp(temp,minTemp,0);           
          break;
          case 2:
            favTemp=changeTemp(temp,favTemp,2); 
          break;
          case 4:
          decidePage();
          break;
        }
        }else{//for scrolled menu
           switch(temp)
        {
          case 0:
              minTemp=changeTemp(temp,minTemp,0); 
          break;
          case 1: 
              favTemp=changeTemp(temp,favTemp,2);       
          break;
          case 2:
          break;
          case 4:
          decidePage();
          break;
        }
        }
        FansControl();
        heatersControl();
        if(isMainMenu){
          showMainMenu();
        }else{
          showScrolledMenu();
        }
        
       lcd.setCursor(0,temp);
       lcd.print('>');        

        waitForKeyRelease();
        break;
        case 'c': //cancel

        break;
    }
    //saving temretures inside eeprom
    EEPROM.write(0,lowByte(favTemp));
    EEPROM.write(1,highByte(favTemp));
    EEPROM.write(2,lowByte(minTemp));
    EEPROM.write(3,highByte(minTemp));
    EEPROM.write(4,lowByte(maxTemp));
    EEPROM.write(5,highByte(maxTemp));
  }
  delay(50);
}


 //---------------------------------------------------------------------
//this method changes tempreture of the enterd tempreture. 
//---------------------------------------------------------------------
int changeTemp(int temp,int tempreture,int isMax)
{
   waitForKeyRelease();
   lcd.clear();
   lcd.setCursor(1,2);
   lcd.print("TEMP is ");
   int t=tempreture;
  if(isMax==2 && (tempreture<minTemp || tempreture>maxTemp)){
    t=(minTemp+maxTemp)/2;
  }
   lcd.print(t);
        while(true){
           char key = getKeyState();
               
              switch(key){
                 case 'u':
                  if(((isMax==0 || (isMax==2)) && t<maxTemp) || (isMax==1)){
                     t++;
                  if(abs(t)==0){
                     lcd.setCursor(10,2);
                     lcd.print(" ");
                  }
              lcd.setCursor(9,2);
              lcd.print(t);
                  }
              waitForKeyRelease();
                break;
                case 'd':
                if(((isMax==1 || (isMax==2)) && t>minTemp) || (isMax==0)){
                    t--;
                if(abs(t)==9){
                     lcd.setCursor(10,2);
                     lcd.print(" ");
                  }
              lcd.setCursor(9,2);
              lcd.print(t);
                }
              waitForKeyRelease();
                break;
                case 'o':
                return t;
                break;
                case 'c':
                 return tempreture;
                break;
              } 
              delay(50);
         }
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
  if(a < 70)
  {
    return 'r'; //right
  }
  if(a < 235)
  {
    return 'l'; //left
  }
  if(a < 415)
  {
    return 'u'; //up    
  }
  if(a < 620)
  {
    return 'd'; //down    
  }
  if(a < 700)
  {
    return 'o'; //select  
  } 
   if(a < 880)
  {
    return 'c'; //cancel 
  } 
  return 0; //none
}


 //---------------------------------------------------------------------
//controlling the cooler 
//if current is 1 degree(or more) more than favorite cooler one turns on
//if current is 3 degree(or more) more than favorite cooler two turns on
//---------------------------------------------------------------------
 void FansControl(){
  if(currTemp>favTemp){
     if(abs(currTemp-favTemp)>=1){
      digitalWrite(12,HIGH);
    }else{
       digitalWrite(12,LOW);
    }
     if(abs(currTemp-favTemp)>=3){
      digitalWrite(11,HIGH);
    }else{
      digitalWrite(11,LOW);
    }
  }else{
    digitalWrite(11,LOW);
    digitalWrite(12,LOW);
  }
 }
 //---------------------------------------------------------------------
//controlling the heaters 
//if current is 1 degree(or more) less than favorite heater one turns on
//if current is 3 degree(or more) less than favorite heater two turns on
//---------------------------------------------------------------------
 void heatersControl(){
  if(currTemp<favTemp){
     if(abs(currTemp-favTemp)>=1){
      digitalWrite(3,HIGH);
    }else{
       digitalWrite(3,LOW);
    }
     if(abs(currTemp-favTemp)>=3){
      digitalWrite(2,HIGH);
    }else{
      digitalWrite(2,LOW);
    }
  }else{
    digitalWrite(2,LOW);
    digitalWrite(3,LOW);
  }
 }

 void buzzerControl(){
    if(currTemp<minTemp || currTemp>maxTemp){
       digitalWrite(10,HIGH);
    }else{
       digitalWrite(10,LOW);
    }
  delay(20);
 }


//---------------------------------------------------------------------
//showing 2 logs in lcd log number i and log number i+1
//---------------------------------------------------------------------
 void showLogs(int i){
       lcd .clear();
       lcd.setCursor(0,0);
       lcd.print("current");
       lcd.setCursor(8,0);
       lcd.print("  ");
       lcd.setCursor(11,0);
       lcd.print("favorite");
       
          lcd.setCursor(2,1);
          lcd.print(logs[i].current);
          lcd.setCursor(14,1);
          lcd.print(logs[i].favorite);
          lcd.setCursor(2,2);
          lcd.print(logs[i+1].current);
          lcd.setCursor(14,2);
          lcd.print(logs[i+1].favorite);
          delay(1);
          
      if(i<8){
      lcd.setCursor(0,3);
      lcd.print("press right->");
      }

 }

//---------------------------------------------------------------------
//change page of logs every time the user clicks the "right" button
//---------------------------------------------------------------------
 void decidePage(){
  int i=0;
  showLogs(i);
  waitForKeyRelease(); 
  while(true){
    byte key=getKeyState();
     switch(key)
    {
       case 'r'://right
       if(sizeof(logs)>=i+2){
           i=i+2;
          if(i>=8){
             lcd .clear();
             lcd.setCursor(0,0);
             lcd.print("no more logs");
             delay(500);
             return;
          }else{
          showLogs(i);
          waitForKeyRelease(); 
          }
       }
        break;
      case 'o': //OK
      case 'c': //cancel
        return;
    }
  }
 }

 
//---------------------------------------------------------------------
//each 2 seconds saving current temp and favorite temp in struct logs
//---------------------------------------------------------------------
void saveLogs(){
unsigned long currTime = millis();
currTemp = (long)analogRead(A1) * 500 / 1024;
  if(currTime >= nextTime)
    {
      logs[countLogs%8].current=currTemp;
      logs[countLogs%8].favorite=favTemp;
      countLogs++;
      nextTime = nextTime + 2000;
    }
delay(1);
 }
  //read tempretures from eeprom
void readFromEEPROM(){
    byte savedFavUpper;
    byte savedFavLower;
    byte savedMinUpper;
    byte savedMinLower;
    byte savedMaxUpper;
    byte savedMaxLower;
    savedFavUpper = EEPROM.read(1);
    savedFavLower = EEPROM.read(0);
    savedMinUpper = EEPROM.read(3);
    savedMinLower = EEPROM.read(2);
    savedMaxUpper = EEPROM.read(5);
    savedMaxLower = EEPROM.read(4);
   minTemp=word(savedMinUpper,savedMinLower);
   maxTemp=word(savedMaxUpper,savedMaxLower);
   favTemp=word(savedFavUpper,savedFavLower);
}



//---------------------------------------------------------------------
//read terminal 
//if enterd temp? then current temp and favorite temp will be printed
//if entered set then user sets favorite temp
//---------------------------------------------------------------------
void readSerial(){
  if(Serial.available()){
        in  = Serial.readString();
        str=str+in;
        Serial.print(in);
       char a[6];
       in.toCharArray(a,6);
        if(a[6]=='\r' || a[4]=='\r'){
        if(str.length()>0){
        if(str.equals("temp?\r")){
          Serial.print("current");
          Serial.print("     ");
          Serial.println("fav");
          Serial.print(currTemp);
          Serial.print("            ");
          Serial.println(favTemp);
        }

         else if(str.equals("set\r")){
          int myNumber=0;
          char c='0';
          Serial.println("enter favorite degree :");
          boolean isNumber=true;
          while(Serial.available()){}
          while(isNumber){
          if(Serial.available()){
             c = Serial.read();
            if(c == '\r'){
                // Number is complete
                Serial.println(myNumber);
                if(myNumber<minTemp || myNumber>maxTemp){
                  Serial.println("Error: number>max or number<min...");
                  Serial.println("enter again...");
                  c='0';
                  myNumber=0;
                }else{
                favTemp=myNumber;
                Serial.println("done");
                isNumber=false;
                }
             }
             else {
                myNumber = myNumber * 10;
                myNumber = myNumber + (c - '0');  // Subtract '0' to adjust from ascii back to real numbers
             }
            }
          }
        }
         
        else{
        Serial.println("wrong input...");//command was not correct
        }
      str="";
      in="";  
  }
  }
  }
  }
//---------------------------------------------------------------------
//updates the main menu and the scrolled menu 
//---------------------------------------------------------------------
  void updateMenu(){
   if(isMainMenu){
        lcd.setCursor(12,3);
        lcd.print(currTemp);
        if(currTemp<10){
        lcd.setCursor(13,3);
        lcd.print(" ");
        } 
        lcd.setCursor(12,2);
        lcd.print(favTemp);
         if(favTemp<10){
        lcd.setCursor(13,2);
        lcd.print(" ");
        } 
    }else{
        lcd.setCursor(12,2);
        lcd.print(currTemp);
        if(currTemp<10){
        lcd.setCursor(13,2);
        lcd.print(" ");
        }
        lcd.setCursor(12,1);
        lcd.print(favTemp);
         if(favTemp<10){
        lcd.setCursor(13,1);
        lcd.print(" ");
        } 
    }
}
