/*
  function command to box :
      commandToBox("openbox", port);
      commandToBox("clear", port);
      commandToBox("request", port);

      #port = 1 - 30
*/
#include <Crc16.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "backup.h"
#include <Wire.h>
#include <QueueArray.h>
#include <Adafruit_SleepyDog.h>
/*----------------( Constant variable )--------------------------*/

#define I2C_ADDRESS 0x50

#define RS485Recieve LOW
#define RS485Transmit HIGH
#define BaudrateRS485 19200
#define TxControlPin 22
#define MasterAddress 0

#define StatusLightPin LED_BUILTIN
/*--------------------( define objects )--------------------------*/

Crc16 crc;
//IPAddress server(172, 18, 9, 157);
const char *server = "nodereddev.kratos.co.th"; //name server mqtt broker
EthernetClient ethClient;
QueueArray<int> queueOpenBox;
/*--------------------( Share variable )--------------------------*/

static byte mac[6];          //keep mac address from EEPROM mac address (i2c)
String tmpMac = "";          //convert byte mac to string mac
char deviceName[20];         //name of subcripe (name as mac)
char master[10] = "/Master"; //pub to mqtt broker
long countLostEvent = 0;     //num of lost event when internet lost connected
char json[200];
unsigned int timeReconnect = 10000;
unsigned long lastTimeReconnect = 0;
unsigned long lastTimeBlinkStatusLight = 0;

uint8_t addressSlave[31];
uint8_t amountSlave = 0;
uint8_t numberOfSlave = 1;
String inputString = "";     // a String to hold incoming data
bool stringComplete = false; // whether the string is complete
bool clearData = false;
bool statusEthernet = false;
bool statusMqtt = false;
bool toggleStatusLight = false;
bool alltest_open = false;
bool queueOpenBox_empty = true;
unsigned int alltest_open_number = 1;
unsigned long lastTimeAlltestOpen = 0;

/*********************(test function)********************/
unsigned long lastTimeToSendStatus = 0;

/*--------------------(Extra functions)------------------*/

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    json[i] = (char)payload[i];
  }
  Serial.println();
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &inputObject = jsonBuffer.parseObject(json);
  if (!inputObject.success())
  {
    Serial.println("parseObject() failed");
    return;
  }
  inputObject.prettyPrintTo(Serial);
  Serial.println();

  String device = inputObject["device"];
  String cmd = inputObject["cmd"];
  String _status = inputObject["status"];
  String response = inputObject["response"];
  int port = inputObject["port"];

  if (device == tmpMac)
  {
    if (cmd == "open")
    {
      // commandToBox("openbox", port);
      queueOpenBox.enqueue(port);
      //      publishMqtt("response", port);
      // Serial.println("Open box " + String(port));
    }
    else if (cmd == "alltest_open")
    {
      alltest_open = true;
      alltest_open_number = 1;
    }
    else if (_status == "open")
    {
      if (response == "ok")
      {
        Serial.println("Send message success");
      }
    }
    else if (_status == "new_message")
    {
      if (response == "ok")
      {
        Serial.println("Send message success");
      }
    }
    else if (_status == "key_switch")
    {
      if (response == "ok")
      {
        Serial.println("Send message success");
      }
    }
    else if (_status == "open_idle")
    {
      if (response == "ok")
      {
        Serial.println("Send message success");
      }
    }
    else if (_status == "close_box")
    {
      if (response == "ok")
      {
        Serial.println("Send message success");
      }
    }
  }
}

PubSubClient client(server, 1883, callback, ethClient);

void initMac()
{
  Serial.print("Getting MAC: ");
  mac[0] = readRegister(0xFA);
  mac[1] = readRegister(0xFB);
  mac[2] = readRegister(0xFC);
  mac[3] = readRegister(0xFD);
  mac[4] = readRegister(0xFE);
  mac[5] = readRegister(0xFF);
  char tmpBuf[17];
  sprintf(tmpBuf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(tmpBuf);
  sprintf(tmpBuf, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  sprintf(deviceName, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  tmpMac = String(tmpBuf);
  Serial.print("tmpmac : ");
  Serial.println(tmpMac);
  Serial.print("deviceName : ");
  Serial.println(deviceName);
}

/*--------------------( Setup )--------------------------*/

void setup()
{
  Serial.begin(9600);
  Serial1.begin(BaudrateRS485);
  Wire.begin();
  pinMode(TxControlPin, OUTPUT);
  pinMode(StatusLightPin, OUTPUT);
  digitalWrite(TxControlPin, RS485Recieve);
  delay(2000);
  initMac();
  sdcardInit();
  connectEthernet();
  delay(500);
  initSlave();
  Watchdog.enable(5000);
}

/*--------------------( Loop )--------------------------*/

void loop()
{
  Watchdog.reset();
  if (!queueOpenBox.isEmpty())
  {
    commandToBox("openbox", queueOpenBox.dequeue());
  }

  if (!statusEthernet)
  {
    blinkEthernetNotConnect();
  }
  if (!statusMqtt)
  {
    blinkMqttBrokerNotConnect();
  }
  if (millis() - lastTimeReconnect >= timeReconnect) //reconnect every 10 sec
  {
    Ethernet.maintain();
    if (ethClient.connected())
    {
      statusEthernet = true;
    }
    else
    {
      statusEthernet = false;
    }
    if (!client.connected())
    {
      reconnectMqtt();
    }
    lastTimeReconnect = millis();
  }

  serialEvent();
  if (stringComplete)
  {
    Serial.println(inputString + "\n");
    processInput(inputString);
    inputString = "";
    stringComplete = false;
  }
  if (alltest_open && (millis() - lastTimeAlltestOpen >= 900))
  {
    if (alltest_open_number >= 30)
    {
      alltest_open = false;
    }
    commandToBox("openbox", alltest_open_number);
    alltest_open_number++;
    lastTimeAlltestOpen = millis();
  }
  else if (!alltest_open)
  {
    if (clearData == false)
    {
      if (numberOfSlave > amountSlave)
      {
        numberOfSlave = 1;
      }

      commandToBox("request", addressSlave[numberOfSlave]);
      numberOfSlave++;
    }
  }
  if (millis() - lastTimeToSendStatus >= 60000)
  {
    publishMqtt("statusBox", 0);
    lastTimeToSendStatus = millis();
  }
  client.loop();
  delay(100);
}

/*--------------------<<<( Functions )>>>-------------------------*/

/*--------------------( initSlave )--------------------------*/

void initSlave()
{

  for (int i = 1; i < sizeof(addressSlave); i++)
  {
    addressSlave[i] = 0;
  }
  amountSlave = checkAmountSlave(); //นับจำนวนบอร์ดลูก
  Serial.println("Amount Slave = " + String(amountSlave));
  for (int i = 1; i < sizeof(addressSlave); i++)
  {
    Serial.print("[" + String(addressSlave[i]) + "] ");
    if (i % 10 == 0)
      Serial.println();
  }
}

/*--------------------( processInput )--------------------------*/

void processInput(String input)
{
  String recieveAddress = input.substring(0, input.indexOf('@'));
  String senderAddress = input.substring(input.indexOf('@') + 1, input.indexOf(','));
  String command = input.substring(input.indexOf(',') + 1, input.indexOf(':'));
  String payload = input.substring(input.indexOf(':') + 1, input.indexOf('#'));
  int tempAddress = recieveAddress.toInt();
  if (tempAddress == MasterAddress)
  {
    char buffer[20];
    if (command.equalsIgnoreCase("R"))
    {
      int newMail = payload.substring(0, 2).toInt();
      int openByKey = payload.substring(2, 4).toInt();
      int close = payload.substring(4, 6).toInt();
      int openIdle = payload.substring(6, 8).toInt();
      if (newMail != 0)
      {
        for (int i = 0; i < newMail; i++)
        {
          publishMqtt("newMessage", senderAddress.toInt());
          delay(200);
        }
        // publishMqtt("newMessage", senderAddress.toInt());
        clearData = true;
      }
      if (openByKey != 0)
      {
        publishMqtt("keySwitch", senderAddress.toInt());
        clearData = true;
      }
      if (close != 0)
      {
        publishMqtt("closeBox", senderAddress.toInt());
        clearData = true;
      }
      if (openIdle != 0)
      {
        publishMqtt("openIdle", senderAddress.toInt());
        clearData = true;
      }
      delay(100);
      if (clearData)
      {
        commandToBox("clear", senderAddress.toInt());
      }
    }
    else if (command.equalsIgnoreCase("O"))
    {
      Serial.print("open send respond : ");
      Serial.println(senderAddress);
      publishMqtt("response", senderAddress.toInt());
    }
    else if (command.equalsIgnoreCase("C"))
    {
      clearData = false;
    }
  }
}

/*--------------------( checkAmountSlave )--------------------------*/

uint8_t checkAmountSlave()
{
  Serial.println("Check amount slave...");
  uint8_t count = 0;
  uint8_t i = 1;
  char buffer[20];
  while (i < sizeof(addressSlave))
  {
    toggleStatusLight = !toggleStatusLight;
    digitalWrite(StatusLightPin, toggleStatusLight);
    sprintf(buffer, "%02d@%02d,CHECK:00#", i, MasterAddress);
    sendMessageRS485(String(buffer));
    int timeOut = 0;
    while (timeOut < 150)
    {
      serialEvent();
      if (stringComplete)
      {
        Serial.println(inputString);
        uint8_t numberSlave = processInputCheckAmountSlave(inputString);
        if (numberSlave != 0)
        {
          addressSlave[count + 1] = numberSlave;
          count++;
        }

        inputString = "";
        stringComplete = false;
        break;
      }
      timeOut++;
      delay(1);
    }
    i++;
    delay(100);
  }
  return count;
}

/*--------------------( processInputCheckAmountSlave )--------------------------*/

uint8_t processInputCheckAmountSlave(String input)
{
  uint8_t number = 0;
  String recieveAddress = input.substring(0, input.indexOf('@'));
  String senderAddress = input.substring(input.indexOf('@') + 1, input.indexOf(','));
  String command = input.substring(input.indexOf(',') + 1, input.indexOf(':'));
  String payload = input.substring(input.indexOf(':') + 1, input.indexOf('#'));
  int tempAddress = recieveAddress.toInt();
  if (tempAddress == MasterAddress)
  {
    if (command.equalsIgnoreCase("CHECK"))
    {
      if (payload.equalsIgnoreCase("ok"))
      {
        number = senderAddress.toInt();
      }
    }
  }

  return number;
}

/*--------------------( serialEvent )--------------------------*/
void serialEvent()
{
  while (Serial1.available())
  {
    char inChar = (char)Serial1.read();
    if (inChar == '\n')
    {
      if (compareCrc(inputString))
      {
        Serial.print("Input pass : ");
        stringComplete = true;
      }
      else
      {
        Serial.println("Input fail");
        inputString = "";
        if (queueOpenBox_empty == false)
        {
          queueOpenBox_empty == true;
        }
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
/*--------------------( commandToBox )------------------------------*/

bool commandToBox(String buf, int number)
{
  char buffer[20];
  bool status = false;
  if (buf.equalsIgnoreCase("openbox"))
  {
    sprintf(buffer, "%02d@00,O:00#", number);
    Serial.println("openBox : " + number);
  }
  else if (buf.equalsIgnoreCase("clear"))
  {
    sprintf(buffer, "%02d@00,C:00#", number);
  }
  else if (buf.equalsIgnoreCase("request"))
  {
    sprintf(buffer, "%02d@00,R:00#", number);
  }
  sendMessageRS485(String(buffer));
}
/*--------------------( sendMessageRS485 )--------------------------*/

void sendMessageRS485(String buf)
{
  while (Serial1.available() > 0)
  {
    byte binDat = Serial1.read();
  }
  char temp[20];
  buf.toCharArray(temp, buf.length() + 1);
  // Serial.println(String(temp));
  sumCrc(temp, buf.length());
  buf += String(crc.getCrc(), HEX) + "\n";
  buf.toCharArray(temp, buf.length() + 1);
  // Serial.println(String(temp));
  digitalWrite(TxControlPin, RS485Transmit);
  Serial1.print(temp);
  Serial1.flush();
  digitalWrite(TxControlPin, RS485Recieve);
  delay(10);
}

/*--------------------( reconnectMqtt )--------------------------*/

void reconnectMqtt()
{
  Serial.print("Attempting MQTT connection...");
  delay(500);
  if (client.connect(deviceName)) // Attempt to connect
  {
    String buf_ip = String(Ethernet.localIP()[0], DEC) + "." + String(Ethernet.localIP()[1], DEC) + "." + String(Ethernet.localIP()[2], DEC) + "." + String(Ethernet.localIP()[3], DEC);
    String buf_subnetmask = String(Ethernet.subnetMask()[0], DEC) + "." + String(Ethernet.subnetMask()[1], DEC) + "." + String(Ethernet.subnetMask()[2], DEC) + "." + String(Ethernet.subnetMask()[3], DEC);
    String buf = "{\"device\":\"" + tmpMac + "\",\"status\":\"signin\",\"ipaddress\":\"" + buf_ip + "\",\"netmask\":\"" + buf_subnetmask + "\"}";
    buf.toCharArray(json, buf.length() + 1);
    Serial.println();
    Serial.println(buf);
    Serial.println("connected");
    client.publish(master, json); // Once connected, publish an announcement...
    buf = "/" + tmpMac;
    buf.toCharArray(json, buf.length() + 1);
    client.subscribe(json);
    checkLostEvent();
    statusMqtt = true;
  }
  else
  {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 3 seconds");
    statusMqtt = false;
  }
}

/*--------------------( connectEthernet )--------------------------*/

void connectEthernet()
{
  // digitalWrite(pinStatusEthernet, LOW);
  Serial.println();
  Serial.print("MAC: ");
  for (byte i = 0; i < 6; ++i)
  {
    Serial.print(mac[i], HEX);
    if (i < 5)
      Serial.print(':');
    delay(10);
  }
  Serial.println();
  Serial.print("Ethernet Connectting DHCP...");
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Connect Fail!! try again in 5 seconds");
    statusEthernet = false;
  }
  else
  {
    Serial.println("Connect Success!!");
    Serial.print("IP  : ");
    Serial.println(Ethernet.localIP());
    Serial.print("Sub : ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("GW  : ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("DNS : ");
    Serial.println(Ethernet.dnsServerIP());
    statusEthernet = true;
  }
}

/*--------------------( publishMqtt )--------------------------*/

void publishMqtt(String cmd, int port)
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &outputObject = jsonBuffer.createObject();
  outputObject["device"] = tmpMac;
  Serial.println("Publish : " + cmd + " box : " + String(port));

  if (cmd == "keySwitch")
  {
    outputObject["status"] = "key_switch";
    outputObject["port"] = String(port);
    // digitalWrite(pinStatusEthernet, LOW);
    // digitalWrite(pinStatusMQTT, LOW);
    // digitalWrite(pinStatusLight, LOW);
    // delay(100);
    // digitalWrite(pinStatusEthernet, HIGH);
    // digitalWrite(pinStatusMQTT, HIGH);
    // digitalWrite(pinStatusLight, HIGH);
  }
  else if (cmd == "newMessage")
  {
    outputObject["status"] = "new_message";
    outputObject["port"] = String(port);
    // digitalWrite(pinStatusEthernet, LOW);
    // digitalWrite(pinStatusMQTT, LOW);
    // digitalWrite(pinStatusLight, LOW);
    // delay(100);
    // digitalWrite(pinStatusEthernet, HIGH);
    // digitalWrite(pinStatusMQTT, HIGH);
    // digitalWrite(pinStatusLight, HIGH);
  }
  else if (cmd == "response")
  {
    outputObject["cmd"] = "open";
    outputObject["port"] = String(port);
    outputObject["response"] = "ok";
    // digitalWrite(pinStatusEthernet, LOW);
    // digitalWrite(pinStatusMQTT, LOW);
    // digitalWrite(pinStatusLight, LOW);
    // delay(100);
    // digitalWrite(pinStatusEthernet, HIGH);
    // digitalWrite(pinStatusMQTT, HIGH);
    // digitalWrite(pinStatusLight, HIGH);
  }
  else if (cmd == "openIdle")
  {
    outputObject["status"] = "open_idle";
    outputObject["port"] = String(port);
    // digitalWrite(pinStatusEthernet, LOW);
    // digitalWrite(pinStatusMQTT, LOW);
    // digitalWrite(pinStatusLight, LOW);
    // delay(100);
    // digitalWrite(pinStatusEthernet, HIGH);
    // digitalWrite(pinStatusMQTT, HIGH);
    // digitalWrite(pinStatusLight, HIGH);
  }
  else if (cmd == "closeBox")
  {
    outputObject["status"] = "close";
    outputObject["port"] = String(port);
    // digitalWrite(pinStatusEthernet, LOW);
    // digitalWrite(pinStatusMQTT, LOW);
    // digitalWrite(pinStatusLight, LOW);
    // delay(100);
    // digitalWrite(pinStatusEthernet, HIGH);
    // digitalWrite(pinStatusMQTT, HIGH);
    // digitalWrite(pinStatusLight, HIGH);
  }
  else if (cmd == "statusBox")
  {
    outputObject["status"] = "alive";
    outputObject["port"] = String(port);
  }

  outputObject.printTo(json, sizeof(json));
  if (client.connected())
  {
    // digitalWrite(pinStatusMQTT, LOW);
    // delay(100);
    // digitalWrite(pinStatusMQTT, HIGH);
    client.publish(master, json);
    // digitalWrite(pinStatusMQTT, LOW);
    // delay(100);
    // digitalWrite(pinStatusMQTT, HIGH);
  }
  else
  {
    countLostEvent++;
    String line = "";
    outputObject.printTo(line);
    backupLostEvent(line, "log.txt");
    removeLostEvent("countlog.txt");
    backupLostEvent(String(countLostEvent), "countlog.txt");
    Serial.println("Have " + String(countLostEvent, DEC) + " event in log.txt");
  }
}

/*--------------------( checkLostEvent )--------------------------*/

void checkLostEvent()
{
  countLostEvent = recieveLostEvent(1, "countlog.txt").toInt();
  if (countLostEvent > 0)
  {
    while (countLostEvent > 0)
    {
      String buf = recieveLostEvent(countLostEvent, "log.txt");
      buf.toCharArray(json, buf.length() + 1);
      if (!client.connected())
      {
        break;
      }
      client.publish(master, json);
      countLostEvent--;
      Serial.println("Send Event Success");
      delay(50);
    }
    removeLostEvent("log.txt");
    removeLostEvent("countlog.txt");
  }
}

/*--------------------( readRegister )--------------------------*/

byte readRegister(byte r)
{
  unsigned char v;
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write(r); // Register to read
  Wire.endTransmission();

  Wire.requestFrom(I2C_ADDRESS, 1); // Read a byte
  while (!Wire.available())
  {
    // Wait
  }
  v = Wire.read();
  return v;
}

/*--------------------(blink Ethernet not connect)---------------------------*/
void blinkEthernetNotConnect()
{
  if (millis() - lastTimeBlinkStatusLight >= 250)
  {
    toggleStatusLight = !toggleStatusLight;
    digitalWrite(StatusLightPin, toggleStatusLight);
    lastTimeBlinkStatusLight = millis();
  }
}
/*--------------------(blink Mqtt not connect)---------------------------*/
void blinkMqttBrokerNotConnect()
{
  if (millis() - lastTimeBlinkStatusLight >= 1000)
  {
    toggleStatusLight = !toggleStatusLight;
    digitalWrite(StatusLightPin, toggleStatusLight);
    lastTimeBlinkStatusLight = millis();
  }
}
