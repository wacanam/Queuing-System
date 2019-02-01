#include "Setup.h";
#include "Json.h";
#include <ArduinoJson.h>

Json client;
Setup  System;

String Fname;
int i = 10;
bool uidFound = false;
String num;
String msg;
signed long nowServing = 1;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(20, 4);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  client.initSD();

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;//keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
  }
  //System.printHex(key.keyByte, MFRC522::MF_KEY_SIZE, ""); //Cause off error

  Serial.println();
  System.GSMint();
  
}

  long st = millis();
void loop()
{
  //uidFound = false;
  System.scan();
  msg = ", Your Priority # is: " + String(System.latest_priority);
  msg = msg + "\nAVG min/trans:" + String(double(System.latest_priority /nowServing ));
  String msg1 = msg + ("\nNow Serving: ") + nowServing;
  if (System.latest_priority == nowServing) {
    msg = msg1 + ("\nProceed now!");
  }
  else if (System.latest_priority >= nowServing + 20 ) {
    String sched = String(myRTC.hours) + ":" + String(myRTC.minutes + 20);
    msg = msg1 + "\nYou can leave and get back by: " + sched ;
  }
  else if (System.latest_priority >= nowServing + 10) {
    String sched = String(myRTC.hours) + ":" + String(myRTC.minutes + 10);
    msg = msg1 + "\nGet back by: " + sched ;
  }
  else {
    msg = msg1 + ("\nPlease Standby");
  }
  if (System.detected) {
    if (System.SMS_TARGET != "") {
      System.sendNotif(System.SMS_TARGET, System.user, msg);
      System.SMS_TARGET = "";
    }
  }
  //search();
  //System.setBlockcontent(num);
  //rw();
  ///////////////////////////
  System.RTC();
  priorityNum();
  //////////////////////////
  if (System.KPad(nowServing)) {
    if (millis() - st > 1000) {
      nowServing++;
      st = millis();
    }
    if(nowServing > System.latest_priority){
      nowServing = System.latest_priority;
      }
  } //Cause of effor
  //search();

}
signed long priorityNum() {
  lcd.setCursor(0, 2);
  lcd.print("Priority #: ");
  lcd.setCursor(11, 2);
  lcd.print(String(System.latest_priority) + " " + nowServing + "     ");
}
bool search() {
  //String uid = "06a2bd57";
  // put your main code here, to run repeatedly:
  if (System.uid != "") {
    while (!uidFound) {
      client.loadJson(i);
      Serial.println("\nSearch: " + System.uid);
      client.parseJson(System.uid);
      if (String(client.NUID) == System.uid) {
        Serial.println(client.Cell);
        num = String(client.Cell);
        uidFound = true;
        return true;
      }
      Serial.println(String("Document: ") + i);
      i++;
      if (i > 15) {
        uidFound = true;
        i = 10;
        Serial.println("No match");
      }
    }
  }
}

void rw()
{

  /*****************************************establishing contact with a tag/card**********************************************************************/

if ( ! rfid.PICC_IsNewCardPresent()) {
    return;/
  }

  // Select one of the cards
  if ( ! rfid.PICC_ReadCardSerial()) {//if PICC_ReadCardSerial returns 1, the "uid" struct (see MFRC522.h lines 238-45)) contains the ID of the read card.
    return;
  }

  Serial.println("card selected");

  /*****************************************writing and reading a block on the card**********************************************************************/

  System.writeBlock(block, blockcontent);//the blockcontent array is written into the card block

  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));//uncomment this if you want to see the entire 1k memory with the block written into it.
  //System.readBlock(block, readbackblock);
  System.readBlock(block, readbackblock);//read the block back
  Serial.print("read block: ");
  for (int j = 0 ; j < 16 ; j++) //print the block contents
  {
    Serial.write(readbackblock[j]);//Serial.write() transmits the ASCII numbers as human readable characters to serial monitor
  }
  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}


