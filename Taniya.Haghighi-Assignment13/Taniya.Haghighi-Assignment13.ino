void setup() {
  // put your setup code here, to run once:
  pinMode(10,OUTPUT);
  pinMode(11,OUTPUT);
  pinMode(12,OUTPUT);
  initTimer1();
}

void loop() {
  // put your main code here, to run repeatedly:

}
void initTimer1()
{
  TCNT1 = 0;
  OCR1A = 26666 - 1; 
  TCCR1A = 0;
  TCCR1B = 0x09; 

  TIMSK1 = (1<<OCIE1A);
  interrupts();
}

 
ISR(TIMER1_COMPA_vect) {
  static byte counter100 = 0;
  static byte counter200 = 0;
  static byte counter300 = 0;  
  
   if(counter300 > 0)
    counter300--;
  else {
    counter300 = 2 - 1;
    do300Times();  
  }
  
  if(counter100 > 0)
    counter100--;
  else {
    counter100 = 6 - 1;
    do100Times();  
  }

  if(counter200 > 0)
    counter200--;
  else {
    counter200 = 3 - 1;
    do200Times();  
  }
}

void do300Times(){
  if(digitalRead(12)==HIGH){
    digitalWrite(12,LOW);
  }else{
    digitalWrite(12,HIGH);
  }
}
void do100Times(){
  if(digitalRead(10)==HIGH){
    digitalWrite(10,LOW);
  }else{
    digitalWrite(10,HIGH);
  } 
}
void do200Times(){
   if(digitalRead(11)==HIGH){
    digitalWrite(11,LOW);
  }else{
    digitalWrite(11,HIGH);
  }
}
