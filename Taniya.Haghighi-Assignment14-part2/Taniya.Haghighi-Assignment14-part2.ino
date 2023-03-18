
#define remote 2
#define Sensor 3

boolean opened=false;
boolean closed =true;

void RIGHT_OPEN();
void LEFT_OPEN();
void BOTH_OPEN();
void RIGHT_CLOSE();
void LEFT_CLOSE();
void BOTH_CLOSE();

void (*stateFunc)(void) = RIGHT_OPEN;
int tOpen=0;
int tClose=0;

void setup() {
  pinMode(remote,INPUT_PULLUP);
  pinMode(Sensor,INPUT_PULLUP);
  pinMode(4,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(0,OUTPUT);
  pinMode(1,OUTPUT);

attachInterrupt(digitalPinToInterrupt(Sensor), openBothDoor, FALLING);
attachInterrupt(digitalPinToInterrupt(remote), remoteOpen, FALLING);
 if(!digitalRead(remote)==LOW){
    stateFunc = BOTH_CLOSE;
  }
}

void remoteOpen(){
   if(!opened){
      stateFunc = RIGHT_OPEN;
  }
}

void openBothDoor(){
  if(!opened && !closed){
      stateFunc = LEFT_OPEN;
  }    
}

void turnOff(int r,int l){
   digitalWrite(r,LOW); 
   digitalWrite(l,LOW);
}

void openDoor(int r,int l){
    digitalWrite(r,HIGH); 
    digitalWrite(l,LOW);
}
void closeDoor(int r,int l){
    digitalWrite(r,HIGH); 
    digitalWrite(l,LOW);
}

void loop() {
  (*stateFunc)();
  delay(500);
}

void RIGHT_OPEN(){
  opened=true;
 closed=false;
  if(tOpen>=4){
    tOpen=0;
    stateFunc = LEFT_OPEN;
  }else{
     openDoor(4,5);
  } 
   tOpen++;
}

void LEFT_OPEN(){
  
  if(tOpen>=30){
    tOpen=0;
    stateFunc = BOTH_OPEN;
  }
  else{
     openDoor(4,5);
     openDoor(0,1);
  } 
  tOpen++;
}

void BOTH_OPEN(){
   if(tOpen>=120 && digitalRead(Sensor)==HIGH){
    tOpen=0;
    tClose=0;
    stateFunc = LEFT_CLOSE;
  }
  turnOff(0,1);
  turnOff(4,5);
   tOpen++;
}

void LEFT_CLOSE(){
  opened=false;
  if(tClose>=4){
    tClose=0;
    stateFunc = RIGHT_CLOSE;
  }else{
     closeDoor(1,0);
  } 
   tClose++;
}


void RIGHT_CLOSE(){

  if(tClose>=30){
    tClose=0;
    stateFunc = BOTH_CLOSE;
  }
  else{
     closeDoor(1,0);
     closeDoor(5,4);
  } 
    tClose++;
}

void BOTH_CLOSE(){
  closed=true;
  turnOff(0,1);
  turnOff(4,5);
}
