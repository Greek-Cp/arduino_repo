#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTClib.h>
#include <Wire.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#define SensorPin A0
SoftwareSerial espSerial(5,6);
//Sensors Ph
float calibration_value = 21.34;
unsigned long int avgValue;
int buf[10], temp ;

RTC_DS3231 rtc;
Servo myservo;
const int sensorPin = 4;
int pinServo = 9;
int sudut = 0;
unsigned long waktuMulai;  // Variabel untuk menyimpan waktu mulai
unsigned long pakanberat; 
// unsigned long durasiPakan;

OneWire oneWire(sensorPin);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  espSerial.begin(115200);
  sensors.begin();
  Wire.begin();
  rtc.begin();
  myservo.attach(pinServo);
  myservo.write(0);
  waktuMulai = millis();
  //sensors Ph
  pinMode(SensorPin, INPUT);

  // Setting waktu awal RTC jika perlu
  rtc.adjust(DateTime(2023, 10, 25, 14, 23, 0));  // Menset waktu awal
  //
}
float phValue; // pH value as a global variable
float temperatureC; // Temperature as a global variable
int year, month, day, hour; // Date and time as global variables
String statusKondisi; // pH status as a global variable
String dataFromESP = "";
int jumlahServoMemutar = 0;
int jmlhDataMasukEsp = 0;
String statusOld = "";
void loop() {
  unsigned long waktuSekarang = millis();

    if (espSerial.available()) {

        // Baca data dan cetak ke Serial Monitor
            // Additional logic here if needed
            dataFromESP = espSerial.readString();
        Serial.println("data dari esp " + dataFromESP);
        int indexColon = dataFromESP.indexOf(':');
        if (indexColon != -1) {
            // Pisahkan string menjadi dua bagian berdasarkan indeks titik dua (:)
             String status = dataFromESP.substring(0, indexColon);
            String putaranServo = dataFromESP.substring(indexColon + 1);
            statusOld = status;

            if(status == "1m"){
              if(statusOld == status){
       int putaranSer = putaranServo.toInt();
                    pakan(putaranSer);
              } else{

              }
             
            // Tampilkan hasil
            } else if(status =="1s"){
              statusOld = "";
            }
            // Konversi string beratPakanStr menjadi integer
      

            // Selanjutnya, Anda dapat menggunakan variabel 'kondisi' dan 'beratPakan' sesuai kebutuhan
        }
        
    }
  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);
  DateTime now = rtc.now();
  // year = now.year();
  // month = now.month();
  // day = now.day();
  // hour = now.hour();
  year = 2023;
  month = 10;
  day = 25;
  hour = 14;
  // Serial.print("tahun :");
  // Serial.print(year);
  // Serial.println("");

  // Serial.print("month :");
  // Serial.print(month);
  // Serial.println("");

  // Serial.print("day :");
  // Serial.print(day);
  // Serial.println("");

  // Serial.print("hour :");
  // Serial.print(hour);
  // Serial.println("");
 

   // Serial.print("Suhuu :");
    // Serial.print(temperatureC);
    // Serial.println("");
    // Serial.println("========================");
  
   //Sensors Ph
   for(int i=0;i<10;i++){
    buf[i]=analogRead(SensorPin);
    delay(15);
  }

  for(int i=0;i<9;i++){ 
    for(int j=i+1;j<10;j++){
      if(buf[i]>buf[j]){
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }

  avgValue=0;
  for(int i=2;i<8;i++)avgValue+=buf[i];
     
  float phValue= (float)avgValue*5.0/1024/6;
  phValue = -5.70 * phValue + calibration_value;
   
  // Serial.print(" pH Value: ");
  // Serial.print(phValue); 
  // Serial.print("  ");
  // Serial.println(phValue);  

  if (phValue < 4)
      statusKondisi = "Very Acidic";
    else if (phValue >= 4 && phValue < 5)
      statusKondisi = "Acidic";
    else if (phValue >= 5 && phValue < 7)
      statusKondisi = "Acidic-ish";
    else if (phValue >= 7 && phValue < 8)
      statusKondisi = "Neutral";
    else if (phValue >= 8 && phValue < 10)
      statusKondisi = "Alkaline-ish";
    else if (phValue >= 10 && phValue < 11)
      statusKondisi = "Alkaline";
    else if (phValue >= 11)
      statusKondisi = "Very alkaline";
    // Serial.print(" pH Status: ");
    // Serial.println(statusKondisi);

  delay(100);
  StaticJsonDocument<200> jsonDocument; // Adjust the size based on your needs

    // Populate the JSON object
  jsonDocument["phValue"] = phValue;
  jsonDocument["temperatureC"] = temperatureC;
  jsonDocument["year"] = year;
  jsonDocument["month"] = month;
  jsonDocument["day"] = day;
  jsonDocument["hour"] = hour;
  jsonDocument["statusKondisi"] = statusKondisi;
  
    // Serialize the JSON object to a string
  String jsonString;
  serializeJson(jsonDocument, jsonString);

    // Print the JSON string to Serial (for debugging)
  //
  Serial.println("JSON Data:");
  Serial.println(jsonString);
  espSerial.println(jsonString);


  // Tambahan kode untuk membaca suhu jika diperlukan.

  delay(5000);
}

void pakan(int jumlahPerulangan) {
  unsigned long waktuMulai = millis();  // Waktu sebelum memulai loop

  for(int i = 0; i < jumlahPerulangan; i++) {
    Serial.print("Putaran ke-");
    Serial.println(i + 1);  // Menampilkan nomor putaran saat ini

    // Urutan gerakan servo
    myservo.write(180);  // Posisi servo 1
    delay(100);
    myservo.write(0);  // Posisi servo 2
    delay(100);
  }

  unsigned long waktuSelesai = millis();  // Waktu setelah loop selesai
  unsigned long durasi = waktuSelesai - waktuMulai;  // Durasi total dalam milidetik

  // Setelah loop selesai, kembalikan servo ke posisi awal atau matikan
  myservo.write(0);

  // Menampilkan durasi total
  Serial.print("Durasi total: ");
  Serial.print(durasi);
  Serial.println(" ms");
}