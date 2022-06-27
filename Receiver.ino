#include <IRremote.h>

//Define Reciever pin
const int RECV_PIN = D5;

IRrecv irrecv(RECV_PIN);
decode_results results; 



void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn();            //Enables Receiver 
}

void loop(){
  //Check for receiving signal 
  if (irrecv.decode(&results))
  {
        Serial.println(results.value);
        irrecv.resume();
  }
}
