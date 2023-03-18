#define sen2 10
#define sen1 11
#define md2 2
#define md1 3

enum FSM_States{OPEN, CLOSE, COMPLETE_OPEN};
FSM_States currentState=OPEN;
 int t=0;

void setup() {
  pinMode(sen1,INPUT_PULLUP);
  pinMode(md2,INPUT_PULLUP);
  pinMode(md1,INPUT_PULLUP);
  pinMode(sen2,INPUT_PULLUP);
  pinMode(4,OUTPUT);
  pinMode(5,OUTPUT);

 

attachInterrupt(digitalPinToInterrupt(md1), openDoor, CHANGE);
attachInterrupt(digitalPinToInterrupt(md2), openDoor, CHANGE);
if(digitalRead(sen2)==LOW){
    currentState = CLOSE;
  }
}

void openDoor(){
        t=0;
         currentState = OPEN;
}

void turnOff(){
   digitalWrite(4,LOW); 
   digitalWrite(5,LOW);
}


void loop() {
  switch(currentState)
  {
    case OPEN: 
        if(digitalRead(sen1)==LOW){
               turnOff();
               t=0;
               currentState = COMPLETE_OPEN;
          }else{
             
                digitalWrite(4,HIGH); 
                digitalWrite(5,LOW);
          } 
        break;
    case COMPLETE_OPEN: 
          t++;
         if(t>4){
          currentState = CLOSE;
         }
        break;
    case CLOSE: 
     if(digitalRead(sen2)==LOW){
      turnOff();
     }else{
      
        digitalWrite(5,HIGH); 
        digitalWrite(4,LOW); 
     }    
  }
  delay(500);
}
