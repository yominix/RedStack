#include <SPI.h>
#include <SD.h>

File backup;

void sdcardInit()
{
  Serial.print("Initializing SD card...");
  if (!SD.begin(4))
  {
    Serial.println("initialization failed!");
  }
  else
  {
    Serial.println("initialization done.");
  }

}
void showLostEvent(String path)
{
  backup = SD.open(path);
  if (backup)
  {
    Serial.println("\n\t\t---" + path + "---");
    while (backup.available())
    {
      Serial.write(backup.read());
    }
    backup.close();
  }
  else
  {
    Serial.println("No have lost event");
  }
}

String recieveLostEvent(int numLine, String path)
{
  String buf = "";
  char tmp;
  backup = SD.open(path);
  if (backup)
  {
    int i = 0;
    Serial.println("Recieve data");
    while (backup.available() && numLine > 0)
    {
      tmp = backup.read();
      Serial.print(tmp);
      if (tmp == '\n')
      {
        numLine--;
      }
      else if (numLine == 1)
      {
        buf += tmp;
        i++;
      }
    }
    Serial.println("Recieve done.");
  }
  else
  {
    Serial.print(path);
    Serial.println(" File not found!!");
  }
  backup.close();
  return buf;
}

void backupLostEvent(String payload, String path)
{
  backup = SD.open(path, FILE_WRITE);
  if (backup)
  {
    Serial.println("Writing to..."+path);
    backup.print(payload);
    backup.print("\n");
    backup.close();
    Serial.println("done.");
  }
  else
  {
    Serial.println("Error opening file!");
  }
}
void removeLostEvent(String path)
{
  backup = SD.open(path);
  Serial.print(path);
  if (backup)
  {
    SD.remove(path);
    Serial.println(" Remove file Success");
  }
  else
  {
    Serial.println(" File not found!!");
  }
  backup.close();
}
void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
void listFile()
{
  delay(250);
  backup = SD.open("/");
  printDirectory(backup, 0);
  Serial.println("done!");
}

