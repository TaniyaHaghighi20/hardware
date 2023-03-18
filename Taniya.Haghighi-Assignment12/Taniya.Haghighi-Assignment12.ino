void setup() {
  pinMode(3, INPUT_PULLUP);
  pinMode(6, OUTPUT);
  invert();
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(3), invert, FALLING);
}
unsigned int counter=-2;
unsigned long nextTime=0;
void loop(){
  unsigned long t = millis();
    if(t >= nextTime)
    {
    digitalWrite(6, LOW);
    nextTime = 0;
    }
    delay(1);
    }

void invert(){
  counter++;
  Serial.println(counter);
  if(counter==20){
    digitalWrite(6, HIGH);
    //delay(20);
    
    counter=0;
    nextTime=millis()+1000;
  }
}
