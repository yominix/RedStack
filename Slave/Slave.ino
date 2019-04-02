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
#define BaudrateRS485 19200
#define masterAddress 0

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
  uint8_t boxClose_uint8;
  bool boxOpenAlert_bool;
};
shareStatus shareDataRS485 = {0, 0, 0, false, false};

String inputString = "";        // a String to hold incoming data
boolean stringComplete = false; // whether the string is complete
bool protectHoldSensorIR = false;
bool protectHoldKeyswitch = false;
bool protectChangeStatusMagnetic = false;
bool alertBoxOpen = false;
bool commandOpenBox = false;
bool toggleBeep = false;
unsigned int timeAlert = 180000; // = 180,000 ms = 180 s = 3 minute
unsigned long lastTimeCheckBoxOpen = 0;
unsigned int timeOpen = 500;
unsigned long lastTimeOpenBox = 0;
unsigned long lastTimeBeep = 0;

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

  digitalWrite(StatusLightPin, HIGH);
  digitalWrite(TxControlPin, RS485Recieve);
  RS485serial.begin(BaudrateRS485);
  inputString.reserve(200);

  shareDataRS485.slaveAddress_unit8 = getAddress();
  Serial.print("#Get slave address = ");
  Serial.println(String(shareDataRS485.slaveAddress_unit8));
}

/*--------------------( Loop )--------------------------*/

void loop()
{
  updateSensor();
  serialEvent();
  if (stringComplete)
  {
    Serial.println(inputString);
    processInput(inputString);
    inputString = "";
    stringComplete = false;
  }
  if (millis() - lastTimeOpenBox >= timeOpen && commandOpenBox)
  {
    digitalWrite(SolenoidPin, LOW);
    commandOpenBox = false;
  }
  // digitalWrite(StatusLightPin, HIGH);
}

/*--------------------<<<( Functions )>>>-------------------------*/

/*--------------------( processInput )--------------------------*/

void processInput(String input)
{
  String recieveAddress = input.substring(0, input.indexOf('@'));
  String senderAddress = input.substring(input.indexOf('@') + 1, input.indexOf(','));
  String command = input.substring(input.indexOf(',') + 1, input.indexOf(':'));
  String payload = input.substring(input.indexOf(':') + 1, input.indexOf('#'));
  int tempAddress = recieveAddress.toInt();
  if (tempAddress == shareDataRS485.slaveAddress_unit8)
  {
    char buffer[20];
    if (command.equalsIgnoreCase("R"))
    {
      sprintf(buffer, "%02d@%02d,R:%02d%02d%02d%02d#", masterAddress, shareDataRS485.slaveAddress_unit8, shareDataRS485.newMail_uint8, shareDataRS485.openByKey_uint8, shareDataRS485.boxClose_uint8, shareDataRS485.boxOpenAlert_bool);
    }
    else if (command.equalsIgnoreCase("C"))
    {
      shareDataRS485 = {shareDataRS485.slaveAddress_unit8, 0, 0, false, false};
      sprintf(buffer, "%02d@%02d,C:ok#", masterAddress, shareDataRS485.slaveAddress_unit8);
    }
    else if (command.equalsIgnoreCase("O"))
    {
      commandOpenBox = true;
      digitalWrite(SolenoidPin, HIGH);
      lastTimeOpenBox = millis();
      sprintf(buffer, "%02d@%02d,O:ok#", masterAddress, shareDataRS485.slaveAddress_unit8);
    }
    else if (command.equalsIgnoreCase("CHECK"))
    {
      sprintf(buffer, "%02d@%02d,CHECK:ok#", masterAddress, shareDataRS485.slaveAddress_unit8, shareDataRS485.slaveAddress_unit8);
    }
    sendMessageRS485(String(buffer));
    digitalWrite(StatusLightPin, HIGH);
    delay(100);
    digitalWrite(StatusLightPin, LOW);
    delay(100);
  }
}

/*--------------------( updateSensor )--------------------------*/

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
    commandOpenBox = true;
    digitalWrite(SolenoidPin, HIGH);
    lastTimeOpenBox = millis();
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
  }
  else if (digitalRead(MagneticPin) == HIGH)
  {
    if (protectChangeStatusMagnetic == true)
    {
      Serial.println("#box close");
      protectChangeStatusMagnetic = false;
      shareDataRS485.boxClose_uint8 += 1;
      delay(500);
    }
    shareDataRS485.boxOpenAlert_bool = false;
    digitalWrite(Buzzerpin, LOW);
    alertBoxOpen = false;
  }
  if (millis() - lastTimeBeep >= 1000 && alertBoxOpen == true)
  {
    Serial.print("..Beep");
    toggleBeep = !toggleBeep;
    digitalWrite(Buzzerpin, toggleBeep);
    lastTimeBeep = millis();
    // delay(250);
    // digitalWrite(Buzzerpin, LOW);
    // delay(250);
  }
}

/*--------------------( getAddress )--------------------------*/

uint8_t getAddress()
{
  uint8_t address = 0;
  if (analogRead(ad1RS485) == LOW)
  {
    Serial.println("+1");
    address += 1;
  }
  if (digitalRead(ad2RS485) == LOW)
  {
    Serial.println("+2");
    address += 2;
  }
  if (digitalRead(ad3RS485) == LOW)
  {
    Serial.println("+4");
    address += 4;
  }
  if (digitalRead(ad4RS485) == LOW)
  {
    Serial.println("+8");
    address += 8;
  }
  if (digitalRead(ad5RS485) == LOW)
  {
    Serial.println("+16");
    address += 16;
  }
  return address;
}

/*--------------------( serialEvent )--------------------------*/

void serialEvent()
{
  while (RS485serial.available())
  {
    // digitalWrite(StatusLightPin, LOW);
    char inChar = (char)RS485serial.read();
    if (inChar == '\n')
    {
      if (compareCrc(inputString))
      {
        Serial.print("Input pass : ");
        stringComplete = true;
      }
      else
      {
        Serial.print("Input fail : ");
        Serial.println(inputString);
        inputString = "";
      }
      break;
    }
    inputString += inChar;
  }
}

/*--------------------( sumCrc )--------------------------*/

void sumCrc(char *dest, int len)
{
  crc.clearCrc();
  for (int i = 0; i <= len; i++)
  {
    crc.updateCrc(dest[i]);
  }
}

/*--------------------( compareCrc )--------------------------*/

bool compareCrc(String strIn)
{
  bool compare = false;
  String inputCrc = inputString.substring(inputString.indexOf('#') + 1);
  String payload = inputString.substring(0, inputString.indexOf('#') + 1);
  // Serial.println("payload = " + payload);
  // Serial.println("input crc = " + inputCrc);
  char buf[20];
  payload.toCharArray(buf, payload.length() + 1);
  sumCrc(buf, payload.length());
  String calCrc = String(crc.getCrc(), HEX);
  // Serial.println("cal crc = " + calCrc);
  if (calCrc.equalsIgnoreCase(inputCrc))
  {
    compare = true;
  }
  return compare;
}

/*--------------------( sendMessageRS485 )--------------------------*/

void sendMessageRS485(String buf)
{
  char temp[20];
  buf.toCharArray(temp, buf.length() + 1);
  // Serial.println(String(temp));
  sumCrc(temp, buf.length());
  buf += String(crc.getCrc(), HEX) + "\n";
  buf.toCharArray(temp, buf.length() + 1);
  // Serial.println(String(temp));
  digitalWrite(TxControlPin, RS485Transmit);
  RS485serial.print(temp);
  RS485serial.flush();
  digitalWrite(TxControlPin, RS485Recieve);
}
