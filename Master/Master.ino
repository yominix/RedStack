#include <Crc16.h>

/*----------------( Constant variable )--------------------------*/

#define RS485Recieve LOW
#define RS485Transmit HIGH
#define BaudrateRS485 4800
#define TxControlPin 22
/*--------------------( define objects )--------------------------*/

Crc16 crc;

/*--------------------( Share variable )--------------------------*/

unsigned long count = 0;
String inputString = "";        // a String to hold incoming data
boolean stringComplete = false; // whether the string is complete

/*--------------------( Setup )--------------------------*/
char buf[] = "^01,R:";
char output[20];
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
  crc.clearCrc();
  for(int i=0;i<sizeof(buf);i++)
  {
    crc.updateCrc(buf[i]);
  }
  unsigned int value = crc.getCrc();
  Serial.println(value,HEX);
  sprintf(output,"^01,R:%X\n",value);
}

/*--------------------( Loop )--------------------------*/

void loop()
{ // run over and over
  delay(1000);

  digitalWrite(TxControlPin, RS485Transmit);
  Serial1.print(output);
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

/*--------------------( Functions )--------------------------*/

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

int calcrc(char *ptr, int count)
{
  int crc;
  char i;
  crc = 0;
  while (--count >= 0)
  {
    crc = crc ^ (int)*ptr++ << 8;
    i = 8;
    do
    {
      if (crc & 0x8000)
        crc = crc << 1 ^ 0x1021;
      else
        crc = crc << 1;
    } while (--i);
  }
  return (crc);
}