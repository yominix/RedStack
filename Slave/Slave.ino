#include <SoftwareSerial.h>

//////----------------พื้นที่สำหรับกำหนดค่าคงที่--------------------------//////

#define RS485Recieve LOW
#define RS485Transmit HIGH
#define RxPin 10
#define TxPin 11
#define TxControlPin 12
#define BaudrateRS485 4800

#define Buzzerpin 6
#define SensorIrPin 5
#define KeyswitchPin 4
#define MagneticPin 3
#define SolenoidPin 2
#define StatusLightPin LED_BUILTIN

//////---------------พื้นที่สำหรับประกาศ object-------------------------////////

SoftwareSerial RS485serial(RxPin, TxPin); // RX, TX

//////----------------พื้นที่สำหรับตัวแปรที่ใช้ร่วมกัน-----------------------///////
typedef struct shareStatus
{
  uint8_t newMail_uint8;
  uint8_t openByKey_uint8;
  bool openBox_bool;
  bool openBoxAlert_bool;
};
shareStatus shareDataRS485 = {12, 15, true, false};

String inputString = "";        // a String to hold incoming data
boolean stringComplete = false; // whether the string is complete
bool protectHoldSensorIR = false;

//////--------------------Setup---------------------------------/////////
void setup()
{
  Serial.begin(9600);
  Serial.println("Goodnight moon!");

  pinMode(SensorIrPin, INPUT);
  pinMode(KeyswitchPin, INPUT);
  pinMode(MagneticPin, INPUT);
  pinMode(Buzzerpin, OUTPUT);
  pinMode(SolenoidPin, OUTPUT);
  pinMode(StatusLightPin, OUTPUT);
  pinMode(TxControlPin, OUTPUT);

  digitalWrite(TxControlPin, RS485Recieve);
  RS485serial.begin(BaudrateRS485);
  inputString.reserve(200);
  char buffer[12];
  sprintf(buffer, "^%d,%02d%02d%02d:", shareDataRS485.newMail_uint8, shareDataRS485.openByKey_uint8, shareDataRS485.openBox_bool, shareDataRS485.openBoxAlert_bool);
  Serial.println(String(buffer));
}
//////--------------------Loop---------------------------------/////////
void loop()
{ // run over and over
  serialEvent();
  if (stringComplete)
  {
    Serial.println(inputString);
    inputString = "";
    stringComplete = false;
  }
}

///////-------------------Functions--------------------------------////////
void serialEvent()
{
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    if (inChar == '\n')
    {
      stringComplete = true;
      break;
    }
    inputString += inChar;
  }
}

void checkSensor()
{
  if (digitalRead(SensorIrPin) == HIGH && protectHoldSensorIR == true)
    protectHoldSensorIR = false;
  if (digitalRead(SensorIrPin) == LOW && protectHoldSensorIR == false)
  {
    
    protectHoldSensorIR = true;
  }
}