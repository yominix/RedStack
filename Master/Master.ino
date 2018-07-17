
#define RS485Recieve LOW
#define RS485Transmit HIGH
#define BaudrateRS485 4800
#define TxControlPin 22

unsigned long count = 0;
String inputString = "";        // a String to hold incoming data
boolean stringComplete = false; // whether the string is complete

void setup()
{
  Serial.begin(9600);
  Serial1.begin(BaudrateRS485);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  pinMode(TxControlPin, OUTPUT);
  digitalWrite(TxControlPin, RS485Recieve);
  Serial.println("Goodnight moon!");
}

void loop()
{ // run over and over
  delay(1000);
  digitalWrite(TxControlPin, RS485Transmit);
  Serial1.print("Say hi! " + String(count) + "\n");
  Serial1.flush();
  digitalWrite(TxControlPin, RS485Recieve);
  delay(100);
  count++;
  serialEvent();
  if (stringComplete)
  {
    Serial.println(inputString);
    inputString = "";
    stringComplete = false;
  }
}
void serialEvent()
{
  while (Serial1.available())
  {
    char inChar = (char)Serial1.read();
    if (inChar == '\n')
    {
      stringComplete = true;
      break;
    }
    inputString += inChar;
  }
}