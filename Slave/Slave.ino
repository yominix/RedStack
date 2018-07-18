#include <SoftwareSerial.h>
#include <Crc16.h>

/*----------------( Constant variable )--------------------------*/

#define ad1RS485 A7
#define ad2RS485 A0
#define ad3RS485 A1
#define ad4RS485 A2
#define ad5RS485 A3

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

/*--------------------( define objects )--------------------------*/

SoftwareSerial RS485serial(RxPin, TxPin); // RX, TX
Crc16 crc;

/*--------------------( Share variable )--------------------------*/

typedef struct shareStatus
{
  uint8_t slaveAddress_unit8;
  uint8_t newMail_uint8;
  uint8_t openByKey_uint8;
  bool boxOpen_bool;
  bool boxOpenAlert_bool;
};
shareStatus shareDataRS485 = {0, 0, 0, false, false};

String inputString = "";        // a String to hold incoming data
boolean stringComplete = false; // whether the string is complete
bool protectHoldSensorIR = false;
bool protectHoldKeyswitch = false;
bool protectChangeStatusMagnetic = false;
bool alertBoxOpen = false;
unsigned long timeAlert = 5000; // = 180,000 ms = 180 s = 3 minute
unsigned long lastTimeCheckBoxOpen = 0;

/*--------------------( Setup )--------------------------*/

void setup()
{
  Serial.begin(9600);
  Serial.println("################## Debug ###################\n");

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

  Serial.print("#Get slave address = ");
  shareDataRS485.slaveAddress_unit8 = getAddress();
  Serial.println(String(shareDataRS485.slaveAddress_unit8));

  //   char buffer[12];
  //   sprintf(buffer, "^%d,%02d%02d%02d:", shareDataRS485.newMail_uint8, shareDataRS485.openByKey_uint8, shareDataRS485.openBox_bool, shareDataRS485.openBoxAlert_bool);
  //   Serial.println(String(buffer));
}

/*--------------------( Loop )--------------------------*/

void loop()
{
  // updateSensor();
  serialEvent();
  if (stringComplete)
  {
    Serial.println(inputString);
    processInput(inputString);
    inputString = "";
    stringComplete = false;
  }
}

/*--------------------( Functions )--------------------------*/
bool processInput(String input)
{
  bool status = false;
  String address = input.substring(1, input.indexOf(','));
  String command = input.substring(input.indexOf(',')+1, input.indexOf(':'));
  int add = address.toInt();
  Serial.println(add);
  Serial.println(command);
  return status;
}

void updateSensor()
{
  /*--------------------( Sensor IR )--------------------------*/
  if (digitalRead(SensorIrPin) == HIGH && protectHoldSensorIR == true)
    protectHoldSensorIR = false;
  if (digitalRead(SensorIrPin) == LOW && protectHoldSensorIR == false)
  {
    if (shareDataRS485.newMail_uint8 < 99)
    {
      shareDataRS485.newMail_uint8 += 1;
    }
    protectHoldSensorIR = true;
    Serial.println("#Sensor active " + String(shareDataRS485.newMail_uint8));
    delay(300);
  }
  /*--------------------( Key switch )--------------------------*/
  if (digitalRead(KeyswitchPin) == HIGH && protectHoldKeyswitch == true)
    protectHoldKeyswitch = false;
  if (digitalRead(KeyswitchPin) == LOW && protectHoldKeyswitch == false)
  {
    if (shareDataRS485.openByKey_uint8 < 99)
    {
      shareDataRS485.openByKey_uint8 += 1;
    }
    protectHoldKeyswitch = true;
    Serial.println("#Key switch active " + String(shareDataRS485.openByKey_uint8));
    delay(300);
  }
  /*--------------------( Magnetic )--------------------------*/
  if (digitalRead(MagneticPin) == LOW)
  {
    if (protectChangeStatusMagnetic == false)
    {
      Serial.println("#box open");
      lastTimeCheckBoxOpen = millis();
      protectChangeStatusMagnetic = true;
      delay(500);
    }
    if (millis() - lastTimeCheckBoxOpen >= timeAlert)
    {
      Serial.println("#box open idel");
      shareDataRS485.boxOpenAlert_bool = true;
      alertBoxOpen = true;
      lastTimeCheckBoxOpen = (millis() - timeAlert) + 3000; //check next 10s
    }
    shareDataRS485.boxOpen_bool = true;
  }
  else if (digitalRead(MagneticPin) == HIGH)
  {
    if (protectChangeStatusMagnetic == true)
    {
      Serial.println("#box close");
      protectChangeStatusMagnetic = false;
      delay(500);
    }
    shareDataRS485.boxOpen_bool = false;
    shareDataRS485.boxOpenAlert_bool = false;
    alertBoxOpen = false;
  }
  if (alertBoxOpen == true)
  {
    Serial.print("..Beep");
    digitalWrite(Buzzerpin, HIGH);
    delay(250);
    digitalWrite(Buzzerpin, LOW);
    delay(250);
  }
}

uint8_t getAddress()
{
  uint8_t address = 0;
  if (digitalRead(ad1RS485) == LOW)
  {
    address += 1;
  }
  if (digitalRead(ad2RS485) == LOW)
  {
    address += 2;
  }
  if (digitalRead(ad3RS485) == LOW)
  {
    address += 4;
  }
  if (digitalRead(ad4RS485) == LOW)
  {
    address += 8;
  }
  if (digitalRead(ad5RS485) == LOW)
  {
    address += 16;
  }
  return address;
}

void serialEvent()
{
  while (RS485serial.available())
  {
    char inChar = (char)RS485serial.read();
    if (inChar == '\n')
    {
      if (compareCrc(inputString))
      {
        Serial.print("Pass CRC : ");
        stringComplete = true;
      }
      else
      {
        inputString = "";
      }
      break;
    }
    inputString += inChar;
  }
}

bool compareCrc(String strIn)
{
  bool compare = false;
  crc.clearCrc();
  String inputCrc = inputString.substring(inputString.indexOf(':') + 1);
  String payload = inputString.substring(0, inputString.indexOf(':') + 1);
  char buf[20];
  payload.toCharArray(buf, payload.length() + 1);
  for (int i = 0; i <= payload.length(); i++)
  {
    crc.updateCrc(buf[i]);
  }
  String calCrc = String(crc.getCrc(), HEX);
  if (calCrc.equalsIgnoreCase(inputCrc))
  {
    compare = true;
  }
  return compare;
}
