#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#include <ArduinoJson.h>
/////For RFID /////////////////
/* Typical pin layout used:
  -----------------------------------------------------------------------------------------
              MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
              Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
  Signal      Pin          Pin           Pin       Pin        Pin              Pin
  -----------------------------------------------------------------------------------------
  RST/Reset   RST          9             24         D9         RESET/ICSP-5     RST
  SPI SS      SDA(SS)      10            22        D10        10               10
  SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
  SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
  SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

#define SPI_PIN 22
#define RST_PIN 24

MFRC522 rfid(SPI_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

// Init array that will store new NUID

int block = 2; //this is the block number we will write into and then read. Do not write into 'sector trailer' block, since this can make the block unusable.

byte blockcontent[16] = {""};//an array with 16 bytes to be written into one of the 64 card blocks is defined!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//byte blockcontent[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //all zeros. This can be used to delete a block.
byte readbackblock[18];//This array is used for reading out a block. The MIFARE_Read method requires a buffer that is at least 18 bytes to hold the 16 bytes of a block.

byte nuidPICC[3];

///////////////////////////////
/////For LCD///////////////////

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

// Creat a set of new characters
const uint8_t charBitmap[][8] = {
  { 0xc, 0x12, 0x12, 0xc, 0, 0, 0, 0 },
  { 0x6, 0x9, 0x9, 0x6, 0, 0, 0, 0 },
  { 0x0, 0x6, 0x9, 0x9, 0x6, 0, 0, 0x0 },
  { 0x0, 0xc, 0x12, 0x12, 0xc, 0, 0, 0x0 },
  { 0x0, 0x0, 0xc, 0x12, 0x12, 0xc, 0, 0x0 },
  { 0x0, 0x0, 0x6, 0x9, 0x9, 0x6, 0, 0x0 },
  { 0x0, 0x0, 0x0, 0x6, 0x9, 0x9, 0x6, 0x0 },
  { 0x0, 0x0, 0x0, 0xc, 0x12, 0x12, 0xc, 0x0 }

};
////////////////////////////////////////
////////////For  Keypad ////////////////
const byte ROWS = 5; //five rows
const byte COLS = 4; //four columns
byte keys[ROWS][COLS] = {
  {'F', 'f', '#', '*'},
  {'1', '2', '3', 'U'},
  {'4', '5', '6', 'D'},
  {'7', '8', '9', 'E'},
  {'L', '0', 'R', 'O'}
};
byte rowPins[ROWS] = {23, 25, 27, 29, 31}; //connect to the row pinouts of the kpd
byte colPins[COLS] = {39, 37, 35, 33}; //connect to the column pinouts of the kpd

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

unsigned long loopCount;
unsigned long startTime;

//////////////////////////////
/////For RTC ////////
//CLK -> 13 , DAT -> 12, Reset -> 11
#include <virtuabotixRTC.h> //Library used for RTC
virtuabotixRTC myRTC(13, 12, 11); //If you change the wiring change the pins here also

///////////////////////////////

//////For GSM///////////
#define TINY_GSM_MODEM_A6
#define TINY_GSM_DEBUG Serial
#include <TinyGsmClient.h>
TinyGsm modem(Serial1);

//////////////////////////////
class Setup {
  public: String SMS_TARGET;
  public: String user;
  public: String uid;
  public: bool detected;
  public: long latest_priority = 0;
  public: byte* setBlockcontent(String content) {
      //blockcontent[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //all zeros. This can be used to delete a block.
      content.getBytes(blockcontent, content.length() + 1);
    }
  public: void printHex(byte *buffer, byte bufferSize) {
      String NUID = "";
      //uid = "";
      for (byte i = 0; i < bufferSize; i++) {
        NUID += String(buffer[i] < 0x10 ? " 0" : " ") + String(buffer[i], HEX);
        //uid += String(buffer[i] < 0x10 ? " 0" : "") + String(buffer[i], HEX);
        //uid.trim();
        //      Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        //      Serial.print(buffer[i], HEX);
      }
      Serial.println(NUID);
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("NUID:" + NUID);
      readBlock(5, readbackblock);
      SMS_TARGET = String((char *)readbackblock);
      readBlock(4, readbackblock);
      user = String((char *)readbackblock);
      Serial.println(user);
      Serial.println(SMS_TARGET);
      if (SMS_TARGET == "") {
        Serial.println("Not Registered");
        return;
      }
      latest_priority++;
    }

    /**
       Helper routine to dump a byte array as dec values to Serial.
    */
    //  public: void printDec(byte *buffer, byte bufferSize) {
    //      String NUID = "";
    //      for (byte i = 0; i < bufferSize; i++) {
    //        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    //        Serial.print(buffer[i], DEC);
    //      }
    //    }
  public: void scan() {
      //Look for new cards
      if ( ! rfid.PICC_IsNewCardPresent())
        return;

      // Verify if the NUID has been readed
      if ( ! rfid.PICC_ReadCardSerial())
        return;

      Serial.print(F("PICC type: "));
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      Serial.println(rfid.PICC_GetTypeName(piccType));

      // Check is the PICC of Classic MIFARE type
      if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
          piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
          piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("Your tag is not of type MIFARE Classic."));
        return;
      }

      if (rfid.uid.uidByte[0] != nuidPICC[0] ||
          rfid.uid.uidByte[1] != nuidPICC[1] ||
          rfid.uid.uidByte[2] != nuidPICC[2] ||
          rfid.uid.uidByte[3] != nuidPICC[3] ) {
        Serial.println(F("A new card has been detected."));
        detected = true;
        // Store NUID into nuidPICC array
        for (byte i = 0; i < 4; i++) {
          nuidPICC[i] = rfid.uid.uidByte[i];
        }

        Serial.println(F("The NUID tag is:"));
        Serial.print(F("In hex: "));
        printHex(rfid.uid.uidByte, rfid.uid.size);
        //        ret = true;
      }
      else {
        Serial.println(F("Card read previously."));
        detected = false;
        tone(10, 500, 700);
      }
      // Halt PICC
      rfid.PICC_HaltA();

      // Stop encryption on PCD
      rfid.PCD_StopCrypto1();
      //      return ret;
    }

    int writeBlock(int blockNumber, byte arrayAddress[])
    {
      for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;//keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
      }

      //this makes sure that we only write into data blocks. Every 4th block is a trailer block for the access/security info.
      int largestModulo4Number = blockNumber / 4 * 4;
      int trailerBlock = largestModulo4Number + 3; //determine trailer block for the sector
      if (blockNumber > 2 && (blockNumber + 1) % 4 == 0) {
        Serial.print(blockNumber);  //block number is a trailer block (modulo 4); quit and send error code 2
        Serial.println(" is a trailer block:");
        return 2;
      }
      Serial.print(blockNumber);
      Serial.println(" is a data block:");

      /*****************************************authentication of the desired block for access***********************************************************/
      byte status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(rfid.uid));
      //byte PCD_Authenticate(byte command, byte blockAddr, MIFARE_Key *key, Uid *uid);
      //this method is used to authenticate a certain block for writing or reading
      //command: See enumerations above -> PICC_CMD_MF_AUTH_KEY_A = 0x60 (=1100000),    // this command performs authentication with Key A
      //blockAddr is the number of the block from 0 to 15.
      //MIFARE_Key *key is a pointer to the MIFARE_Key struct defined above, this struct needs to be defined for each block. New cards have all A/B= FF FF FF FF FF FF
      //Uid *uid is a pointer to the UID struct that contains the user ID of the card.
      if (status != MFRC522::STATUS_OK) {
        Serial.print("PCD_Authenticate() failed: ");
        //Serial.println(mfrc522.GetStatusCodeName(status));
        return 3;//return "3" as error message
      }
      //it appears the authentication needs to be made before every block read/write within a specific sector.
      //If a different sector is being authenticated access to the previous one is lost.


      /*****************************************writing the block***********************************************************/

      status = rfid.MIFARE_Write(blockNumber, arrayAddress, 16);//valueBlockA is the block number, MIFARE_Write(block number (0-15), byte array containing 16 values, number of bytes in block (=16))
      //status = mfrc522.MIFARE_Write(9, value1Block, 16);
      if (status != MFRC522::STATUS_OK) {
        Serial.print("MIFARE_Write() failed: ");
        //Serial.println(mfrc522.GetStatusCodeName(status));
        return 4;//return "4" as error message
      }
      Serial.println("\nBlock was written");
    }


    int readBlock(int blockNumber, byte arrayAddress[])
    {
      for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;//keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
      }

      int largestModulo4Number = blockNumber / 4 * 4;
      int trailerBlock = largestModulo4Number + 3; //determine trailer block for the sector

      /*****************************************authentication of the desired block for access***********************************************************/
      byte status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(rfid.uid));
      //byte PCD_Authenticate(byte command, byte blockAddr, MIFARE_Key *key, Uid *uid);
      //this method is used to authenticate a certain block for writing or reading
      //command: See enumerations above -> PICC_CMD_MF_AUTH_KEY_A = 0x60 (=1100000),    // this command performs authentication with Key A
      //blockAddr is the number of the block from 0 to 15.
      //MIFARE_Key *key is a pointer to the MIFARE_Key struct defined above, this struct needs to be defined for each block. New cards have all A/B= FF FF FF FF FF FF
      //Uid *uid is a pointer to the UID struct that contains the user ID of the card.
      if (status != MFRC522::STATUS_OK) {
        Serial.print("PCD_Authenticate() failed (read): ");
        //Serial.println(mfrc522.GetStatusCodeName(status));
        return 3;//return "3" as error message
      }
      //it appears the authentication needs to be made before every block read/write within a specific sector.
      //If a different sector is being authenticated access to the previous one is lost.


      /*****************************************reading a block***********************************************************/

      byte buffersize = 18;//we need to define a variable with the read buffer size, since the MIFARE_Read method below needs a pointer to the variable that contains the size...
      status = rfid.MIFARE_Read(blockNumber, arrayAddress, &buffersize);//&buffersize is a pointer to the buffersize variable; MIFARE_Read requires a pointer instead of just a number
      if (status != MFRC522::STATUS_OK) {
        Serial.print("MIFARE_read() failed: ");
        //Serial.println(mfrc522.GetStatusCodeName(status));
        return 4;//return "4" as error message
      }
      Serial.println("\nBlock was read");
    }

  public: void RTC() {
      myRTC.updateTime();
      lcd.setCursor(0, 3);
      lcd.print(myRTC.dayofmonth); //You can switch between day and month if you're using American system
      lcd.print("/");
      lcd.print(myRTC.month);
      lcd.print("/");
      lcd.print(myRTC.year);
      lcd.setCursor(12, 3);
      lcd.print("");
      lcd.print(myRTC.hours);
      lcd.print(":");
      lcd.print(myRTC.minutes);
      lcd.print(":");
      lcd.print(myRTC.seconds);
    }
  public: bool KPad(long i) {
      bool plus = false;
      loopCount++;
      if ( (millis() - startTime) > 5000 ) {
        lcd.setCursor(0, 0);
        //lcd.clear();
        lcd.print("AVG min/trans: ");
        lcd.print(String(double(latest_priority / i)) + "     ");
        startTime = millis();
        loopCount = 0;
      }
      if (kpd.getKeys())
      {
        for (int i = 0; i < LIST_MAX; i++) // Scan the whole key list.
        {
          if (kpd.key[i].kchar == 'O') {
            return 1;
          }
          if ( kpd.key[i].stateChanged )   // Only find keys that have changed state.
          {
            String msg;
            switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
              case PRESSED:
                msg = " PRESSED";
                break;
              case HOLD:
                msg = " HOLD";
                break;
              case RELEASED:
                msg = " RELEASED";
                break;
              case IDLE:
                msg = " IDLE";
                break;
            }
            String k = "";
            switch (kpd.key[i].kchar) {
              case 'F':
                k = "F1";
                break;
              case 'f':
                k = "F2";
                break;
              case 'U':
                k = "UP";
                break;
              case 'D':
                k = "DOWN";
                break;
              case 'E':
                k = "ESC";
                break;
              case 'O':
                k = "OK";
                break;
              case 'L':
                k = "LEFT";
                break;
              case 'R':
                k = "RIGHT";
                break;
              default:
                k = kpd.key[i].kchar;
                break;
            }

            lcd.setCursor(0, 0);
            lcd.clear();
            lcd.print("Key ");
            lcd.print(k);
            lcd.print(msg);
          }
        }
      }
    }

  public: void GSMint() {
      static uint32_t rate = 0;
      lcd.setCursor(0, 1);
      lcd.print("Please Standby");
      lcd.setCursor(0, 2);
      lcd.print("Connecting to GSM") + rate;
      if (!rate) {
        rate = TinyGsmAutoBaud(Serial1);
      }

      if (!rate) {
        Serial.println(F("***********************************************************"));
        Serial.println(F(" Module does not respond!"));
        Serial.println(F("   Check your Serial wiring"));
        Serial.println(F("   Check the module is correctly powered and turned on"));
        Serial.println(F("***********************************************************"));
        lcd.clear();
        lcd.setCursor(0, 2);
        lcd.print("ERROR GSM");
        return;
      }

      Serial1.begin(rate);

      // Access AT commands from Serial Monitor
      Serial.println(F("***********************************************************"));
      Serial.println(F(" You can now TAP your tags"));
      Serial.println(F("***********************************************************"));
      lcd.clear();
      lcd.setCursor(0, 2);
      lcd.print("GSM Ready");
    }
  public: void sendNotif(String SMS_TARGET, String user, String msg) {
      tone(10, 1000, 200);
      lcd.setCursor(0, 0);
      lcd.print("Sending to:" + user);
      String txt = String(user + msg);
      Serial.println(txt);
      bool res = modem.sendSMS(SMS_TARGET, String(user + msg));
      Serial.print("SMS:" + String( res ? "OK" : "fail"));
      lcd.print((SMS_TARGET + "-SMS:") + String( res ? "OK" : "fail"));
      if (!res) {
        latest_priority -= 1;
      }
      RTC();
    }
};
