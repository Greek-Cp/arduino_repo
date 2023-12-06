#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <SoftwareSerial.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
SoftwareSerial ardunoSerial(5, 6);

#define WIFI_SSID "arduinohp"
#define WIFI_PASSWORD "12345678"
#define hr hour
#define min minute
#define sec second
#define dy day
#define mth month
#define yr year
#define API_KEY "AIzaSyAyYHslDtCAwP55zIpFGBCqwDTHPWDwTok"
#define DATABASE_URL "https://lele-wise-5f987-default-rtdb.asia-southeast1.firebasedatabase.app"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
const int GMT_OFFSET_SECONDS = 7 * 3600;

unsigned long lastFirebaseUpdate = 0;
const unsigned long firebaseUpdateInterval = 1000;

unsigned long waktuMulai;  
unsigned long pakanberat; 

int count = 0;
int beratPakan = 0;
bool signupOK = false;
bool feedingOccurred = false;
time_t feedingTime = 0;
 int feedingDelay = 0; // Delay 2 menit

int indexCallPakanFunction = 0;
void setup()
{
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  feedingOccurred = false;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  timeClient.begin();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

FirebaseJson firebaseJson;
unsigned long lastTime = 0;
const long interval = 5000 ;
String markTime;
FirebaseJson firebaseJsonRiwayat;


String indexChild = "";
String tanggalPakan = "";
String beratPakanGlobal = "";
void insertRiwayatPakan(String waktuPakan) {

    // Menyiapkan data untuk dikirim
    firebaseJsonRiwayat.clear();
    firebaseJsonRiwayat.add("waktu_pakan", waktuPakan);
    firebaseJsonRiwayat.add("tanggal", tanggalPakan);
    firebaseJsonRiwayat.add("berat_pakan",beratPakanGlobal);

    // Mengirim data ke Firebase
    String childKey = "riwayat_" + String(millis());
    if (Firebase.ready() && (millis() - lastFirebaseUpdate > firebaseUpdateInterval || lastFirebaseUpdate == 0)) {
        lastFirebaseUpdate = millis();

        if (!Firebase.RTDB.setJSON(&fbdo, "/riwayat_pakan/" + childKey, &firebaseJsonRiwayat)) {
            // Handle error
        }
    } 
}
bool compareTime(String jsonTime,String tanggal,String indexRef) 
{
    tmElements_t tm;
    int _hour, _minute;
    sscanf(jsonTime.c_str(), "%d:%d", &_hour, &_minute);
    tm.Hour = _hour;
    tm.Minute = _minute;
    timeClient.update();
    time_t utcUnixTime = timeClient.getEpochTime();
    time_t gmtUnixTime = utcUnixTime + GMT_OFFSET_SECONDS;

    int jam = hour(gmtUnixTime);
    int menit = minute(gmtUnixTime);

    String jamSekarang = String(jam) + ":" + String(menit);
    String jadwalMakan = String(_hour) + ":" + String(_minute);
    // Serial.println("jam sekarang : " + jamSekarang);
    // Serial.println("jam jadwal makanan : " + jadwalMakan);
  //  Serial.print("feedingoccured : ");
    Serial.println(feedingOccurred);
    if (jamSekarang == jadwalMakan && !feedingOccurred)
    {
        feedingOccurred = true;
        feedingTime = now();
        markTime = jamSekarang;
        // Serial.println("Feeding time started");
       float gramPerPutaran = 0.138; 
float jumlahPutaran = beratPakan / gramPerPutaran;
// Serial.print("Berat pakan : ");
// Serial.println(beratPakan);
// Serial.print("jumlah putaran : ");
// Serial.println(jumlahPutaran);
float waktuPkn = jumlahPutaran / 100;
        feedingDelay = waktuPkn * 20;
        indexCallPakanFunction++;
        if(indexCallPakanFunction == 1){
          pakan(jumlahPutaran);
          indexChild = indexRef;
          beratPakanGlobal = String(beratPakan);
          tanggalPakan = tanggal;
        //  Serial.println(indexRef);
        }

        return true;
    }
    else if (feedingOccurred)
    {
        time_t timeLeft = feedingDelay - (now() - feedingTime);
        if (timeLeft <= 0)
        {
                Serial.println("1s:0");
                    insertRiwayatPakan(jamSekarang);
               
                    Serial.println(indexChild);
                
    String path = "/konfigurasi_pakan/konfigurasi_pakan/" + indexChild;
  //  Serial.println(path);
                    deleteItem(path);
                    
            feedingOccurred = false;
            // Serial.println("Feeding time ended, delay elapsed");
            indexCallPakanFunction = 0; 
            return true;
        }
        else
        {
            if(jadwalMakan == markTime){
        
           // Serial.print("In feeding delay, time left: ");
           // Serial.print(timeLeft);
          //  Serial.println(" seconds");
                        return true; // Mengembalikan true selama masa delay
            } else {
            // Serial.print("Not Feeding");
            // Serial.print(timeLeft);
            // Serial.println(" seconds");
              return false;
            }

        }
    }
    //Serial.println("No feeding event");
    return false;
}

void deleteItem(String path) {
 
    if (Firebase.RTDB.deleteNode(&fbdo, path)) {
//        Serial.println("Item deleted successfully. with index");
        
    } else {
  //      Serial.println("Failed to delete item: " + fbdo.errorReason());
    }
}
void loop()
{
  timeClient.update();
  unsigned long currentTime = millis();

  if (currentTime - lastTime >= interval)
  {
    lastTime = currentTime;

    if (Firebase.ready())
    {
      if (Firebase.RTDB.getArray(&fbdo, "/konfigurasi_pakan/konfigurasi_pakan"))
      {
        String waktuPakan;

        for (size_t i = 0; i < fbdo.jsonArray().size(); i++)
        {
          FirebaseJsonData jsonData;
          fbdo.jsonArray().get(jsonData, i);

          if (jsonData.success)
          {
            DynamicJsonDocument doc(1024);
            waktuPakan = jsonData.to<String>();
            deserializeJson(doc, waktuPakan);
            beratPakan = doc["berat_pakan"]; // Use the global variable
            String pengulangan = doc["pengulangan"].as<String>();
            String tanggal = doc["tanggal"].as<String>();
            String waktuPakan1 = doc["waktu_pakan"].as<String>();
            String indexAsString = String(i);
        //           Serial.print("Index: ");
        // Serial.println(indexAsString);
        // Serial.print("Pengulangan: ");
        // Serial.println(pengulangan);
        // Serial.print("Tanggal: ");
        // Serial.println(tanggal);
        // Serial.print("Waktu Pakan: ");
        // Serial.println(waktuPakan1);
            if (compareTime(waktuPakan1,tanggal,indexAsString))
            {
            
            }
          }
        }
      }
      else
      {
      // Serial.println("Firebase Get Array Error: " + fbdo.errorReason());
      }
    }
  }

  if (Serial.available())
  {
    String jsonDariArduino = Serial.readString();
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonDariArduino);

    if (error)
    {
      // Handle error
      return;
    }

    float phValue = doc["phValue"];
    float temperatureC = doc["temperatureC"];
    int year = doc["year"];
    int month = doc["month"];
    int day = doc["day"];
    int hour = doc["hour"];
      String statusKondisi = doc["statusKondisi"];

      firebaseJson.clear();
      firebaseJson.add("ph", phValue);
      firebaseJson.add("temperature", temperatureC);
      firebaseJson.add("year", year);
      firebaseJson.add("month", month);
      firebaseJson.add("day", day);
      firebaseJson.add("hour", hour);
      firebaseJson.add("status_kondisi", statusKondisi);

    if (Firebase.ready() && signupOK && (millis() - lastFirebaseUpdate > firebaseUpdateInterval || lastFirebaseUpdate == 0))
    {
      lastFirebaseUpdate = millis();
  
    String childKey = "datariwayat" + String(millis());
      // if (!Firebase.RTDB.setJSON(&fbdo, "/datariwayat/" + childKey, &firebaseJsonRiwayat)) {
      //       // Handle error
      //   }
      if (!Firebase.RTDB.setJSON(&fbdo, "/datafirebase", &firebaseJson))
      {
        // Handle error
      }
    }
  }
}

void pakan(int jumlahPutaran) {
    Serial.print("1m:");
    Serial.println(jumlahPutaran);
  
}