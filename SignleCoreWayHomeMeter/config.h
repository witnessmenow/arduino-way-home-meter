// ----------------------------
// WiFi Details
// ----------------------------
#define MYSSID "Your_SSID"       // your network SSID (name)
#define MYPASS "Your_PASS"  // your network key

// ----------------------------
// API Keys & Tokens
// ----------------------------
#define TELEGRAM_BOT_TOKEN "123456789sdfghjkl45rt6789fghjk"
#define MAPS_API_KEY "123456789sdfghjkl45rt6789fghjk"


#define HOME_LOCATION "dublin,ireland"

#define SERVO_PIN D1

#define BUZZER_PIN D3

#define NEOPIXEL_PIN D2
#define NUMPIXELS 11

// ----------------------------
// LED Matrix config
// ----------------------------

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define NUMBER_OF_DEVICES 4

// Wiring for an ESP8266
// We are using the hardware SPI pins so we don't need
// to define them other than CS_PIN, 
// but I'll leave here for reference
#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D8  // or SS

// This impacts how fast the text scrolls on the dsiplay,
// increase the delay to make it scroll slower
#define MATRIX_ANIM_DELAY  100  // in milliseconds


// ----------------------------
// TimeZones - The clock auto adjusts for daylight savings
// ----------------------------

// From "WorldClock" example in timezone library.
// NOTE: The examples for this library will be under a 
// "INCOMPATABLE" folder when you have the ESP8266 board selected

// Timezone for UK and Ireland
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};        // British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};         // Standard Time
Timezone timezone(BST, GMT);


// Here are the other timezones from the example, enable which ever one applies.
// Remebmer to comment out the above one!

//// Australia Eastern Time Zone (Sydney, Melbourne)
//TimeChangeRule aEDT = {"AEDT", First, Sun, Oct, 2, 660};    // UTC + 11 hours
//TimeChangeRule aEST = {"AEST", First, Sun, Apr, 3, 600};    // UTC + 10 hours
//Timezone timezone(aEDT, aEST);
//
//// Moscow Standard Time (MSK, does not observe DST)
//TimeChangeRule msk = {"MSK", Last, Sun, Mar, 1, 180};
//Timezone timezone(msk);
//
//// Central European Time (Frankfurt, Paris)
//TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
//TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
//Timezone timezone(CEST, CET);
//
//
//// UTC
//TimeChangeRule utcRule = {"UTC", Last, Sun, Mar, 1, 0};     // UTC
//Timezone timezone(utcRule);
//
//// US Eastern Time Zone (New York, Detroit)
//TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC - 4 hours
//TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC - 5 hours
//Timezone timezone(usEDT, usEST);
//
//// US Central Time Zone (Chicago, Houston)
//TimeChangeRule usCDT = {"CDT", Second, Sun, Mar, 2, -300};
//TimeChangeRule usCST = {"CST", First, Sun, Nov, 2, -360};
//Timezone timezone(usCDT, usCST);
//
//// US Mountain Time Zone (Denver, Salt Lake City)
//TimeChangeRule usMDT = {"MDT", Second, Sun, Mar, 2, -360};
//TimeChangeRule usMST = {"MST", First, Sun, Nov, 2, -420};
//Timezone timezone(usMDT, usMST);
//
//// Arizona is US Mountain Time Zone but does not use DST
//Timezone timezone(usMST);
//
//// US Pacific Time Zone (Las Vegas, Los Angeles)
//TimeChangeRule usPDT = {"PDT", Second, Sun, Mar, 2, -420};
//TimeChangeRule usPST = {"PST", First, Sun, Nov, 2, -480};
//Timezone timezone(usPDT, usPST);

