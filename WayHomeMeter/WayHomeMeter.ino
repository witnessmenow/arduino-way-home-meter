/*******************************************************************
    Way Home Meter

    Use Telegram and Google Maps to display the live travel
    time from your current location to home.

    Runs on an ESP32. Uses an MAX7219 LED Matrix to display the
    the travel time and servo to represent the distance

    By Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Twitch: https://www.twitch.tv/brianlough
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// ----------------------------
// Standard Libraries - Already Installed if you have ESP32 set up
// ----------------------------

#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include "LedMatrix.h"
// The driver for the LED Matrix Display
// Using my fork of Daniel Eichorn's library (For support for software SPI and rotation)
// https://github.com/witnessmenow/MAX7219LedMatrix

#include <UniversalTelegramBot.h>

#include <Adafruit_NeoPixel.h>

#include <GoogleMapsApi.h>

#include <ArduinoJson.h>

#include <ESP32Servo.h>

#include <NTPClient.h>

#include <Timezone.h>    // https://github.com/JChristensen/Timezone Modified!


//------- Replace the following! ------
char ssid[] = WIFI_NAME;       // your network SSID (name)
char password[] = WIFI_PASS;  // your network key

#define TELEGRAM_BOT_TOKEN "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#define MAPS_API_KEY "XXXXXXXXXXXXXXXXXXXXXXXXXXX"

String homeCoOrds = "dublin,ireland"; //Co-Ords work too

// From World clock example in timezone library
// United Kingdom (London, Belfast)
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};        // British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};         // Standard Time
Timezone UK(BST, GMT);

unsigned long delayBetweenChecks = 60000; //mean time between api requests
unsigned long whenDueToCheck = 0;

WiFiClientSecure client;

Servo myservo;

UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, client);
GoogleMapsApi mapsApi(MAPS_API_KEY, client);

String homeCoOrds = "dublin,ireland";

// ------ LED Matrix ------
#define NUMBER_OF_DEVICES 4

// Wiring that works with ESP32
#define CS_PIN 15
#define CLK_PIN 14
#define MISO_PIN 2 //we do not use this pin just fill to match constructor
#define MOSI_PIN 12 // Labeled as DIN

#define SERVO_PIN 18

#define NEOPIXEL_PIN 25
#define NUMPIXELS 11

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

String timeString;
bool showTime = true;

String screenMessage;
bool newData = false;

int freq = 2000;
int channel = 1;
int resolution = 8;

int servoPos = 0;

boolean isFirstLocationRecieved = true;

TaskHandle_t Task1;

void driveDisplay(void * parameter) {
  while (true) {
    ledMatrix.clear();
    if (showTime) {
      ledMatrix.setTextAlignment(TEXT_ALIGN_LEFT);
      ledMatrix.setText(timeString);
      ledMatrix.drawText();
      ledMatrix.commit();
      delay(35);
    } else {
      ledMatrix.scrollTextLeft();
      ledMatrix.drawText();
      ledMatrix.commit();
      delay(35);
      if (newData) {
        newData = false;
        ledMatrix.setNextText(screenMessage);
      }
    }
  }
}

void setup() {

  Serial.begin(115200);

  pixels.begin();
  turnOffNeopixels();

  pinMode(17, OUTPUT);
  ledcSetup(channel, freq, resolution);
  //ledcAttachPin(17, channel);
  //ledcWrite(channel, 0);

  myservo.attach(SERVO_PIN, 500, 2400);

  delay(100);
  myservo.write(0);
  servoPos = 0;
  delay(500);
  myservo.detach();

  ledMatrix.init();
  ledMatrix.setRotation(true);
  ledMatrix.setText("Way Home Meter");
  ledMatrix.setIntensity(8);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  timeClient.begin();

  bot.longPoll = 60;

  xTaskCreatePinnedToCore(
    driveDisplay,            /* Task function. */
    "DisplayDrive",                 /* name of task. */
    1000,                    /* Stack size of task */
    NULL,                     /* parameter of the task */
    1,                        /* priority of the task */
    &Task1,                   /* Task handle to keep track of created task */
    0);                       /* Core */
}

int trafficValue = 0;
int distanceValue = 0;

String getTravelTime(String origin) {
  String responseString = mapsApi.distanceMatrix(origin, homeCoOrds, "now", "best_guess");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& response = jsonBuffer.parseObject(responseString);
  if (response.success()) {
    if (response.containsKey("rows")) {
      JsonObject& element = response["rows"][0]["elements"][0];
      String status = element["status"];
      if (status == "OK") {

        String distance = element["distance"]["text"];
        String duration = element["duration"]["text"];
        String durationInTraffic = element["duration_in_traffic"]["text"];
        trafficValue = element["duration_in_traffic"]["value"];
        distanceValue = element["distance"]["value"];

        return durationInTraffic;

      }
      else {
        Serial.println("Got an error status: " + status);
      }
    } else {
      Serial.println("Reponse did not contain rows");
    }
  } else {
    Serial.println("Failed to parse Json");
  }

  return "Error getting TravelTime";
}

int startingDistance = 0;

void getTelegramData() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  if (numNewMessages) {
    if (bot.messages[0].type == "callback_query") {
      screenMessage = bot.messages[0].text;
      newData = true;
    } else if (bot.messages[0].longitude != 0 || bot.messages[0].latitude != 0) {
      String origin = String(bot.messages[0].latitude, 7) + "," + String(bot.messages[0].longitude, 7);
      //Serial.println(origin);
      Serial.println("Updated");
      String chat_id_one = String(bot.messages[0].chat_id);
      bot.sendMessage(chat_id_one, "Updated", "");
      screenMessage = getTravelTime(origin);
      newData = true;

      if (isFirstLocationRecieved) {
        isFirstLocationRecieved = false;
        startingDistance = distanceValue;
        generateTone();
      } else if (distanceValue < 5) {
        //finish Mode
        isFirstLocationRecieved = true;
        generateTone();
        screenMessage = "Nearly Home!";
      }

      int distanceTraveled = startingDistance - distanceValue;
      int traveled = (distanceTraveled * 10 / startingDistance); // We want to move in tenths

      turnOffNeopixels();
      delay(200);
      updateNeopixel(traveled);
      updateServo(traveled);


    } else
    {
      String text = bot.messages[0].text;
      String chat_id = String(bot.messages[0].chat_id);
      if (text == "/options") {
        String keyboardJson = "[[{ \"text\" : \"This button will say Hi\", \"callback_data\" : \"Hi\" }],[{ \"text\" : \"This will say Bye\", \"callback_data\" : \"Bye\" }]]";
        bot.sendMessageWithInlineKeyboard(chat_id, "Choose from one of the following options", "", keyboardJson);
      } else {
        screenMessage = text;
        newData = true;
      }
    }
  }
}

void updateServo(int traveled) {
  Serial.println("Updating Servo");
  int calculatedServoPos = 180 - (traveled * 18);
  Serial.print("startingDistance: ");
  Serial.println(startingDistance);
  Serial.print("distanceValue: ");
  Serial.println(distanceValue);
  Serial.print("calculatedServoPos: ");
  Serial.println(calculatedServoPos);
  if (servoPos != calculatedServoPos) {
    generateTone();
    myservo.attach(SERVO_PIN, 500, 2400);
    Serial.print("Updating Servo Pos");
    int modifier = 1;
    if (calculatedServoPos < servoPos ) {
      modifier = -1;
    }
    int pos = 0;
    for (pos = servoPos; pos != calculatedServoPos; pos += modifier) { // goes from 0 degrees to 180 degrees
      // in steps of 1 degree
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(15);                       // waits 15ms for the servo to reach the position
    }
    //    for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    //      // in steps of 1 degree
    //      myservo.write(pos);              // tell servo to go to position in variable 'pos'
    //      delay(15);                       // waits 15ms for the servo to reach the position
    //    }
    //    for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    //      myservo.write(pos);              // tell servo to go to position in variable 'pos'
    //      delay(15);                       // waits 15ms for the servo to reach the position
    //    }
    //myservo.write(calculatedServoPos);
    servoPos = calculatedServoPos;
    myservo.detach();
  }
}

void generateTone() {
  ledcAttachPin(17, channel);
  ledcWrite(channel, 125);
  ledcWriteTone(channel, 1000);
  delay(500);
  ledcDetachPin(17);
}

void updateNeopixel(int traveled) {
  for (int i = 0; i <= traveled; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    pixels.show();
    delay(200);
  }
}

void turnOffNeopixels() {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    pixels.show();
  }
}

String getFormattedTime(unsigned long rawTime) {
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr;
}

void loop() {
  unsigned long timeNow = millis();
  timeClient.update();
  if (timeNow > whenDueToCheck) {
    unsigned long epoch = UK.toLocal(timeClient.getEpochTime());
    Serial.print("epoch ");
    Serial.print(epoch);
    timeString = getFormattedTime(epoch);
    getTelegramData();
    timeNow = millis();
    whenDueToCheck = timeNow + 1000;

  }
}
