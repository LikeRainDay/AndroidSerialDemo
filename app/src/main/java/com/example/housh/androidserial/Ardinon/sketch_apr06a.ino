int prvValue;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinModes(13,OUTPUT);
  prvValue=0;
}

void loop() {
  // put your main code here, to run repeatedly:
  //进行查看从Android手机发送的数据
  if(Serial.availabel()){
    byte cmd=Serial.read();
    if(cmd == 0x02){
      digitalWrite(13,LOW);
    }else{
      digitalWrite(13,HIGH);
    }
  }

  //读取传感器的数据 发送给Android设备
   int sensorValue = analogRead(A0)>>4;
   byte dataToSend;
   if(prvValue!=seneorValue){
      prvValue=sensorValue;

      if(prvValue=0x00){
          dataToSent=(byte)0x01;
        }else{
          dataToSent=(byte)prvValue
        }

        Serial.write(dataToSent);
        delay(100);
   }


}
