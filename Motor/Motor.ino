#include <Servo.h>
Servo myservo;
int pos = 0;
int readin = 0;

int state = 20;
String txtMsg = "";

void setup() {
  // initialize both serial ports:
  myservo.attach(8);
  Serial1.begin(9600);
  Serial.begin(9600);

  while (!Serial1) {
    ;
  };
  Serial1.println("AT+CSQ");
  ClearData();
  Serial1.println("AT*MCGDEFCONT=\"IP\",\"internet.iot\"");
  ClearData();
  Serial1.println("AT+CSTT=\"internet.iot\"");
  ClearData();
  Serial1.println("AT+CGCONTRDP");
  ClearData();
  //Serial1.println("AT+CIICR");
  //ClearData();
  Serial1.println("AT+CMQNEW=\"220.134.220.50\",\"1883\",12000,100");
  ClearData();
  Serial1.println("AT+CMQCON=0,3,\"myclient\",600,0,0");
  ClearData();
  Serial1.println("AT+CMQSUB=0,\"feed\",1");
  ClearData();
  //Serial1.println("AT+CMQPUB=0,\"feed\",1,0,0,2,\"30\"");
  //ClearData();
}

void loop() {

  /*
    while(Serial.availabe()){
    String mqttread = Serial.read();
    }
  */
  while (Serial1.available()) {
    String a = Serial1.readString();
    Serial.print(a[30]);
    if (a[30] == '1')
    {
      for (int i = 0; i <= 180; i += 1) {
        myservo.write(i);
        delay(5);
      }
      for (int i = 180; i >= 0; i -= 1) {
        myservo.write(i);
        delay(5);
      }
      Serial1.println("AT+CMQPUB=0,\"feed\",1,0,0,2,\"30\"");
      ClearData();
    }

  }

  /*
    //馬達
    while(Serial.available()>0)
    {
      //String presetText = "+CMQPUB: 0,\"feed\",0,0,0,2,\"31\"";
      //Serial.println(presetText[28]);
      readin = Serial.read();
      if(readin=='1')
      {
        for(int i = 0; i <= 180; i+=1){
          myservo.write(i);
          delay(5);
        }
        for(int i = 180; i >= 0; i-=1){
          myservo.write(i);
          delay(5);
          }
            readin=0;
        }
    //Serial1.println("AT+CMQPUB=0,\"feed\",1,0,0,2,\"30\"");
    //ClearData();
    }




  */






  if (Serial1.available())
  {
    int inByte = Serial1.read();
    Serial.write(inByte);
  }

  /*
      Serial.println("AT+CMQPUB=0,\"feed\",1,0,0,2,\"30\"");


      // read from port 1, send to port 0:
      if (Serial.available()) {
        int inByte = Serial.read();
        Serial1.write(inByte);
      }
      // read from port 0, send to port 1:
      if (Serial1.available()) {
        int inByte = Serial1.read();
        Serial.write(inByte);
      }*/
}

void ClearData()
{
  unsigned long pushTime = millis();
  while (millis() - pushTime < 5000)
  {

    if (Serial.available())
    {
      int inByte = Serial.read();
      Serial1.write(inByte);
    }

    if (Serial1.available())
    {
      int inByte = Serial1.read();
      Serial.write(inByte);
    }
  }
}
