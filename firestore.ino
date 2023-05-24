//Libraries

#include<Firebase_ESP_Client.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

//Firebase Defination
#define API_KEY "AIzaSyA-UD9AkPMalYkEY6D7RQoifOZC6e2LXr8"
#define PROJECT_ID "access-control-system-aa610"
#define UEMAIL "xxx@gmail.com"
#define PASSWORD "xxx"
#define FIREBASE_FCM_SERVER_KEY "KEY"
#define DEVICE_TOKEN "eYcVYnddSle4vfnocWBJyc:APA91bFAkoik2cP_nvMNQp8pTZ0vwkLCPG6MT6Oh0cWBPElYbBeBOerZfGb8EkDEc8nbL0CSWlERKOmxiDbXrCE700BEmMyCbFTMG217_ZIfI8rkcevxtiyWGFtBvVqQ-YsuML6hvQr4"

#define WIFI_SSID "RTE_HALL"
#define WIFI_PASSWORD ""

const long utcOffsetInSeconds = 20700;

//MCq2 ko lagi
#define mq2Input A0
float smoke;

//RFID
#define SS_PIN 15
#define RST_PIN 0

String MasterTag = "7374B9E";
String tagID = "";
MFRC522 mfrc522(SS_PIN, RST_PIN);

//Relay
#define relay_pin 4

//servo
#define servo_pin 5
#define close 90
#define open 0
Servo myservo;

//defined for delay
unsigned long previousTime;
unsigned long currentTime;
unsigned long delayInterval = 300000;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


FirebaseConfig config;
FirebaseData fbdo;
FirebaseAuth auth;

void doorStatusClose(){
  String documentPath = "users/RUVXqXwcahPXjtXMEgqq1vDjTWv2";
  FirebaseJson content;
  content.set("fields/doorStatus/stringValue","Close");
  Firebase.Firestore.patchDocument(&fbdo,PROJECT_ID,"",documentPath.c_str(),content.raw(), "doorStatus");
}
void doorStatusOpen(){
  String documentPath = "users/RUVXqXwcahPXjtXMEgqq1vDjTWv2";
  FirebaseJson content;
  content.set("fields/doorStatus/stringValue","Open");
  Firebase.Firestore.patchDocument(&fbdo,PROJECT_ID,"",documentPath.c_str(),content.raw(), "doorStatus");
}
void lightStatusOn(){
  String documentPath = "users/RUVXqXwcahPXjtXMEgqq1vDjTWv2";
  FirebaseJson content;
  content.set("fields/lightStatus/stringValue","On");
  Firebase.Firestore.patchDocument(&fbdo,PROJECT_ID,"",documentPath.c_str(),content.raw(), "lightStatus");
}
void lightStatusOff(){
  String documentPath = "users/RUVXqXwcahPXjtXMEgqq1vDjTWv2";
  FirebaseJson content;
  content.set("fields/lightStatus/stringValue","Off");
  Firebase.Firestore.patchDocument(&fbdo,PROJECT_ID,"",documentPath.c_str(),content.raw(), "lightStatus");
}
void smokeDataUpdate(float x){
  String documentPath = "users/RUVXqXwcahPXjtXMEgqq1vDjTWv2";
  char smokeValue[5];
  dtostrf(x, 0, 2, smokeValue);
  FirebaseJson content;
  content.set("fields/smokePercent/stringValue",smokeValue);
  Firebase.Firestore.patchDocument(&fbdo,PROJECT_ID,"",documentPath.c_str(),content.raw(), "smokePercent");
}

boolean readDoorStatus(){
  String documentPath = "users/RUVXqXwcahPXjtXMEgqq1vDjTWv2";
  
  // Read existing document data
  FirebaseJson documentData;
  if (Firebase.Firestore.getDocument(&fbdo, PROJECT_ID, "", documentPath.c_str(),"doorStatus, lightStatus")) {
    {
      documentData.setJsonData(fbdo.payload());
      FirebaseJsonData doorStatusData;
      documentData.get(doorStatusData, "fields/doorStatus/stringValue");
      if(doorStatusData.success){
        if(doorStatusData.to<String>()=="Close"){
          return 0;
        }
        else{
          return 1;
        }
      }
    
    }
  } else {
    return 0;
  }
  return 0;
}
boolean readLightStatus(){
  String documentPath = "users/RUVXqXwcahPXjtXMEgqq1vDjTWv2";

  // Read existing document data
  FirebaseJson documentData;
  if (Firebase.Firestore.getDocument(&fbdo, PROJECT_ID, "", documentPath.c_str(),"doorStatus, lightStatus")) {
    {
      documentData.setJsonData(fbdo.payload());
      FirebaseJsonData lightStatusData;
      documentData.get(lightStatusData, "fields/lightStatus/stringValue");
      if(lightStatusData.success){
        if(lightStatusData.to<String>()=="Off"){
          return 0;
        }
        else{
          return 1;
        }

      }
    }
  } else {
    return  0;
  }
  return 0;
}

void sendMessage()
{

    timeClient.update();

    String time = timeClient.getFormattedTime();

    FCM_Legacy_HTTP_Message msg;

    msg.targets.to = DEVICE_TOKEN;

    msg.options.time_to_live = "1000";
    msg.options.priority = "high";

    msg.payloads.notification.title = "Smoke Alert";
    msg.payloads.notification.body = "There is smoke alert";

    FirebaseJson payload;

    // all data key-values should be string
    payload.add("title", "Smoke Alert");
    payload.add("message", "Smoke detector has detected smoke and it has crossed its threshold");
    payload.add("time", time);
    msg.payloads.data = payload.raw();
    Firebase.FCM.send(&fbdo, &msg);

}





void setup() {
   Serial.begin(9600); 
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

   while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
    }

  auth.user.email = UEMAIL;
  auth.user.password = PASSWORD;
  config.api_key = API_KEY;
  Firebase.begin(&config, &auth);
  Firebase.FCM.setServerKey(FIREBASE_FCM_SERVER_KEY);


  //Hardware codes
  SPI.begin();

  myservo.attach(servo_pin);
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, LOW);
  pinMode(mq2Input, INPUT);
  mfrc522.PCD_Init();
  delay(4);
  //for delay
  previousTime = millis();
}

void loop() 
{                                                                                      
  //For Smoke sensor
  smoke = analogRead(mq2Input);
  smoke = smoke/1024;
  smokeDataUpdate(smoke);
  if(smoke>=0.5){
    sendMessage();// Perform action here
  }

  //Door and Light
  //rfid
    while(getID()){
    Serial.println(tagID);
    if (tagID == MasterTag){
      digitalWrite(relay_pin, HIGH);
      myservo.write(open);  // move servo to 90 degrees
      lightStatusOn();
      doorStatusOpen();
      delay(10000);  // wait for 10 second
      myservo.write(close);  // move servo to 0 degrees
      doorStatusClose();
      }}

  //app
  //for door
  if(readDoorStatus()){
    Serial.println("DoorOn");
      myservo.write(open);
  }
  else if(!readDoorStatus()){
    myservo.write(close);
    Serial.println("DoorOFf");
  }
  
  //for relay
  if(readLightStatus()){
      digitalWrite(relay_pin, HIGH);
  }
  else if(!readLightStatus()){
    digitalWrite(relay_pin, LOW);
  }

}




boolean getID() 
{

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }

  //Since a PICC placed get Serial and continue
  if ( ! mfrc522.PICC_ReadCardSerial()) {
  return false;
  }
  tagID = "";

  for ( uint8_t i = 0; i < 4; i++) {
  //readCard[i] = mfrc522.uid.uidByte[i];
  // Adds the 4 bytes in a single String variable
  tagID.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  tagID.toUpperCase();
  mfrc522.PICC_HaltA(); // Stop reading
  return true;

}
