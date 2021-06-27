/*******************************************************************
    Way Home Meter

    Use Telegram and Google Maps to display the live travel
    time from your current location to home.

    Runs on an ESP8266. Uses an MAX7219 LED Matrix to display the
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
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <Servo.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <UniversalTelegramBot.h>
// The library for connecting with Telegram Messenger Bot API
// Search for "Telegram" in the Arduino Library manager
// https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot

#include <GoogleMapsApi.h>
// The library for connecting with the Google Maps API
// Search for "Google Maps" in the Arduino Library manager
// https://github.com/witnessmenow/arduino-google-maps-api

#include <ArduinoJson.h>
// Library used for parsing Json from the API responses
// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

#include <MD_Parola.h>
// The library for adding animations to the display
// Search for "Parola" in the Arduino Library manager
// https://github.com/MajicDesigns/MD_Parola

#include <MD_MAX72xx.h>
// The library for controlling the MAX7219 LED Matrix
// Search for "MD_MAX72xx" in the Arduino Library manager
// https://github.com/MajicDesigns/MD_MAX72xx

#include <Adafruit_NeoPixel.h>
// Library for controlling the Neopixels
// Search for "neopixel" in the Arduino Library manger
// https://github.com/adafruit/Adafruit_NeoPixel

#include <NTPClient.h>
// Library for getting the time from the internet
// Search for "NTPClient"(by Fabrice Weinberg)in the library manager
// https://github.com/arduino-libraries/NTPClient

#include <Timezone.h>
// Library for automatically adjusting the time for daylight savings
// Search for "timezone" on the library manager
// https://github.com/JChristensen/Timezone
// NOTE: You will get a warning that this is only for AVR boards,
// this can be ignored.

// ----------------------------
// Dependancy Libraries - used by one of the above libraies.
// ----------------------------

#include <TimeLib.h>
// Time is a library that provides timekeeping functionality for Arduino.
// Search for "time" on the library manager, Author is: "Michael Margolis" (it's a pain to find...)
// https://github.com/PaulStoffregen/Time

// ----------------------------
// Config
// ----------------------------

// You will need to change the values in "config.h" for this
// sketch to work.
#include "config.h"

char ssid[] = MYSSID;       // your network SSID (name)
char password[] = MYPASS;  // your network key

const unsigned long delayBetweenChecks = 2000; //Time between requests to Telegram
unsigned long whenDueToCheck = 0;

const unsigned long delayBetweenGoogleMapChecks = 60000 * 2; //Time between requests to Google Maps
unsigned long whenDueToCheckGoogleMaps = 0;

WiFiClientSecure client;
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, client);
GoogleMapsApi mapsApi(MAPS_API_KEY, client);

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, NUMBER_OF_DEVICES);

Servo myservo;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

String timeString;

String screenMessage;
bool newData = false;

int servoPos = 0;

boolean isFirstLocationRecieved = true;

String lastTimeString = "";

void setup() {

  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pixels.begin();
  turnOffNeopixels();

  //myservo.attach(SERVO_PIN, 500, 2400);
  myservo.attach(SERVO_PIN);

  delay(100);
  myservo.write(0);
  servoPos = 0;
  delay(500);
  myservo.detach();

  P.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(MYSSID, MYPASS);
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

  client.setInsecure();

  bot.longPoll = 5;
}

void displayTime(String text) {
  P.setTextAlignment(PA_CENTER);
  P.print(text);
}

void scrollText(String text)
{
  char buffer[text.length() + 1];
  text.toCharArray(buffer, text.length() + 1);
  P.displayScroll(buffer, PA_CENTER, PA_SCROLL_LEFT, 60);
  while (!P.displayAnimate()) {
    delay(1);
  }
  P.displayScroll(buffer, PA_CENTER, PA_SCROLL_LEFT, 60);
  while (!P.displayAnimate()) {
    delay(1);
  }
}


int trafficValue = 0;
int distanceValue = 0;

bool getTravelTime(String origin) {
  String responseString = mapsApi.distanceMatrix(origin, HOME_LOCATION, "now", "best_guess");
  DynamicJsonDocument doc(10000); // 10000 is being pulled out thin air...
  DeserializationError error = deserializeJson(doc, responseString);
  if (!error) {
    if (doc.containsKey("rows")) {
      JsonObject element = doc["rows"][0]["elements"][0];
      if (element["status"] == "OK") {

        trafficValue = element["duration_in_traffic"]["value"];
        distanceValue = element["distance"]["value"];

        return true;

      }
      else {
        Serial.print("Got an error status: ");
        //String status = element["status"];
        //Serial.println(status);
      }
    } else {
      Serial.println("Reponse did not contain rows");
    }
  } else {
    Serial.print("deserializeJson() failed with code ");
    Serial.println(error.c_str());
  }

  return false;
}

int startingDistance = 0;

String lastRecievedCoOrds;
String senderName;
bool wayHomeActive = false;

void getTelegramData() {

  //Serial.println("getting Telegram data");
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  //Serial.println(numNewMessages);
  if (numNewMessages) {
    //Serial.println("Handeling Message");
    String chat_id_one = String(bot.messages[0].chat_id);
    if (bot.messages[0].type == "callback_query") {
      //Serial.println("Is a callback");
      if (wayHomeActive && bot.messages[0].text == "/stop") {
        bot.sendMessage(chat_id_one, "Stopped Tracking, turn off live location share or it will start again", "");
        resetWayHomeMeter("Tracking was stopped by user");
      }

    } else if (bot.messages[0].longitude != 0 || bot.messages[0].latitude != 0) {
      //Serial.println("Seems to be a location");
      lastRecievedCoOrds = String(bot.messages[0].latitude, 7) + "," + String(bot.messages[0].longitude, 7);
      Serial.println("Got Co-Ords");

      senderName = bot.messages[0].from_name;

      if (isFirstLocationRecieved) {
        String keyboardJson = "[[{ \"text\" : \"Stop Tracking\", \"callback_data\" : \"/stop\" }]]";
        bot.sendMessageWithInlineKeyboard(chat_id_one, "Way Home Meter is now tracking", "", keyboardJson);
        senderName = bot.messages[0].from_name;
        whenDueToCheckGoogleMaps = 0;
        wayHomeActive = true;
        isFirstLocationRecieved = false;

      }

    } else
    {
      //Serial.println("Displaying Message");
      screenMessage = bot.messages[0].text;
      newData = true;
    }
  }
}
int previousCoOrdsSameCount;
void resetWayHomeMeter(String messageToDisplay)
{
  wayHomeActive = false;
  isFirstLocationRecieved = true;
  screenMessage = messageToDisplay;
  newData = true;

  updateServo(10);
  turnOffNeopixels();

  previousCoOrdsSameCount = 0;
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
    //generateTone();
    myservo.attach(SERVO_PIN);
    //myservo.attach(SERVO_PIN, 500, 2400);
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
  tone(BUZZER_PIN, 2000, 500);
  delay(700);
  noTone(BUZZER_PIN);
  delay(350);
}

void updateNeopixel(int traveled) {
  Serial.print("Update Neopixel:");
  Serial.println(traveled);
  for (int i = 0; i <= traveled; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 150, 0));
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

unsigned long hours;
unsigned long minutes;
unsigned long seconds;

// Converts Epoch time to a String in hh:mm format
String getFormattedTime(unsigned long rawTime) {
  hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr;
}

// Adds the esitmated travel time to the current time and returns a
// String in the hh:mm format.
String getArrivalTime() {

  // Travel time starts in seconds
  int travelTimeInMins =  trafficValue / 60;

  // Adding the travel time in minutes on to the current time (we'll fix if it goes over 59 below)
  unsigned long arrivalMinute = minutes + travelTimeInMins;

  // Take the example where we were at the time 18:30 with a travel time of 120 mins
  // arrivalMinute would become 150

  // if arrival minute is divisible by 60, that value should be
  // carried to the hours
  // From our example: 150 / 60 = 2 (not 2.5, because it's an int)
  unsigned long arrivalHour = hours + (arrivalMinute / 60);

  // Getting the remainder of dividing by 60 and making that the new minute value
  // We have already added the additonal time required to the hour above
  // From our example: 150 % 60 = 30 (FYI: 30 % 60 = 30 )
  arrivalMinute = arrivalMinute % 60;
  String minuteStr = arrivalMinute < 10 ? "0" + String(arrivalMinute) : String(arrivalMinute);

  // Handling if we have gone above 24 for the hour value
  arrivalHour = arrivalHour % 24;
  String hoursStr = arrivalHour < 10 ? "0" + String(arrivalHour) : String(arrivalHour);

  return hoursStr + ":" + minuteStr;
}

String getWayHomeText() {
  String arrivalTime = getArrivalTime();
  return senderName + " will be home at " + arrivalTime;
  if (distanceValue < 5) {
    screenMessage = "Nearly Home!";
  }
}

void handleTimeUpdate() {
  unsigned long epoch = timezone.toLocal(timeClient.getEpochTime());
  Serial.print("epoch ");
  Serial.print(epoch);
  Serial.printf("1Available RAM: %d\n", ESP.getFreeHeap());
  timeString = getFormattedTime(epoch);
  displayTime(timeString);
}

String previousSentCoOrds;

void handleGoogleMapsUpdate() {
  if (previousSentCoOrds == lastRecievedCoOrds) {
    previousCoOrdsSameCount++;
    if (previousCoOrdsSameCount > 5) {
      resetWayHomeMeter("No new location recieved, stopping tracking");
      return;
    }
  }


  getTravelTime(lastRecievedCoOrds);
  previousSentCoOrds = lastRecievedCoOrds;
  screenMessage = getWayHomeText();
  newData = true;
  if (startingDistance == 0) {
    generateTone();
    startingDistance = distanceValue;
  }
  int distanceTraveled = startingDistance - distanceValue;
  int traveled = (distanceTraveled * 10 / startingDistance); // We want to move in tenths

  turnOffNeopixels();
  delay(200);
  updateNeopixel(traveled);
  updateServo(traveled);
}

void loop() {
  unsigned long timeNow = millis();
  timeClient.update();

  // Every second update the time and check Telegram for new messages.
  if (timeNow > whenDueToCheck) {
    handleTimeUpdate();
    Serial.printf("2Available RAM: %d\n", ESP.getFreeHeap());
    delay(50);
    getTelegramData();
    timeNow = millis();
    whenDueToCheck = timeNow + delayBetweenChecks;

    Serial.printf("3Available RAM: %d\n", ESP.getFreeHeap());

  }

  timeNow = millis();

  // If WayHome is active, when its due find the travel time from
  // Google maps.
  if (wayHomeActive && timeNow > whenDueToCheckGoogleMaps) {
    handleGoogleMapsUpdate();

    whenDueToCheckGoogleMaps = timeNow + delayBetweenGoogleMapChecks;

  }

  // If there is new data to display, update the screen
  // then return to the time.
  if (newData) {
    newData = false;
    scrollText(screenMessage);
    displayTime(timeString);
  }
}
