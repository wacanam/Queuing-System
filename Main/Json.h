File file;
class Json {

  public: const char* Fname;
//  public: const char* Mname;
//  public: const char* Lname;
//  public: const char* Gender;
  public: const char* ID;
  public: const char* Cell;
  public: const char* NUID;
    String jsons;
    String filename;

    //Json doc = new Json();
  public: void initSD() {
      if (!SD.begin()) {
        Serial.println("initialization failed!");
        return;
      }
    }
  public: void loadJson(int i) {
      filename = String(i);
      file = SD.open(filename + ".txt");
      jsons = "";
      jsons = file.readString();
      file.close();
    }
  public: bool parseJson(String uid) {
      const size_t capacity = JSON_ARRAY_SIZE(10) + 10 * JSON_OBJECT_SIZE(7) + 1252;
      DynamicJsonBuffer jsonBuffer(capacity);
      JsonArray& root = jsonBuffer.parseArray(jsons);
      if (!root.success()) {
        Serial.println("Unable to Parse JSON");
      }
      for (JsonObject& i : root) {
        Fname = i["First_Name"];
        //Mname = i["Middle_Name"];
        //Lname = i["Last_Name"];
        //Gender = i["Gender"];
        ID = i["ID_Number"];
        Cell = i["Mobile_Number"];
        NUID = i["NUID"];
        Serial.println(Fname);
        //Serial.println(Mname);
        ////Serial.println(Lname);
        //Serial.println(Gender);
        Serial.println(ID);
        Serial.println(Cell);
        Serial.println(NUID);
        Serial.println();
        Serial.flush();
        if (uid == String(NUID)) {
          Serial.println("Found: " + String(NUID));
          Serial.println("Match to: " + uid);
          break;
        }
      }
    }
};
