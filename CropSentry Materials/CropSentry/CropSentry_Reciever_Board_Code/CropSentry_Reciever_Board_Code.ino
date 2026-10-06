#include <heltec_unofficial.h>
#include <WiFiManager.h>
#include <HTTPClient.h>


#define PAUSE 300
#define FREQUENCY 905.2
#define BANDWIDTH 500.0
#define SPREADING_FACTOR 9
#define TRANSMIT_POWER 0

// Lines 10-11 copied from WiFiClentConnect WiFi.h library

volatile bool packetReady = false;
String data;
int a = 0;  // Flag for an issue with the sending system taking too many attempts to repair
int b = 0;  // Flag for an issue with seeing if info was ever sent
int c = 0;  // Flag for if network needs reconnection without full reset
unsigned long startTime = 0;
unsigned long waitLength = 0;
bool waiting = false;


void IRAM_ATTR onPacket() {
  packetReady = true;
}

void setup() {
  // put your setup code here, to run once:
  heltec_setup();
  Serial.begin(115200);
  WiFiManager wm;
  heltec_ve(true);  // Uses on-board VEXT pin for the displays 3V3 power source
  delay(100);
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);
  delay(50);
  digitalWrite(21, HIGH);
  delay(50);  // Sets pin 21 (display controller) LOW and HIGH for power configuration

  display.init();
  display.flipScreenVertically();

  both.println("Hold PRG to \n Reset WiFi");
  delay(2000);
  if (digitalRead(BUTTON) == LOW) {
    wm.resetSettings();
  }
  display.clear();

  both.println("Connect to the Board \n WiFi System. \n You will be prompted \n with further instructions");

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("CropSentry WiFi Portal"); // anonymous ap
  res = wm.autoConnect("CropSentry-Module-WiFi", "password");  // password protected ap

  if (!res) {
    both.println("Failed to connect \n Please try again \n Restarting...");
    delay(2000);
    ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    both.println("WiFi Connected \n Proceeding...");
  }

  // Lines 28 and 41-53 pasted from the WifiManager.h library

  both.println("Radio Initiating...");
  RADIOLIB_OR_HALT(radio.begin());
  // Set the callback function for received packets
  radio.setDio1Action(onPacket);
  // Set radio parameters
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  // Start receiving

  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));


  both.println("WiFi Receiver Board \n Activated \n Starting Processing...");
  delay(2500);
}

void wifiSetup() {
  WiFiManager wm;
  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("CropSentry WiFi Portal"); // anonymous ap
  res = wm.autoConnect("CropSentry-Module-WiFi", "password");  // password protected ap

  if (!res) {
    both.println("Failed to connect \n Please try again \n Restarting...");
    delay(2000);
    ESP.restart();
  } else {
    //if you get here you have connected to the WiFi
    both.println("WiFi Connected \n Proceeding...");
  }
}
void sendServer() {
  if (a >= 5) {
    both.println("Sending System Damaged \n Please Repair");
    while (1) delay(1);
  }
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  http.begin("https://httpbin.org/post");  // Link to server
  Serial.print("[HTTP] POST...\n");
  // start connection and send HTTP header
  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST(data);  // Sends STR var from sensor module to server
                                   // httpCode will be negative on error


  if (httpCode < 0) {
    both.println("Network Failure \n Error Code Sub-0 \n Trying Again...");
    http.end();
    c = 1;
    a++;


  } else if (httpCode == 404 || httpCode == 400) {
    both.println("Server Code Failure \n Error Code 404/400 \n Ending Processes...");
    http.end();
    while (1) delay(1);


  } else if (httpCode == 408 || httpCode == 429) {
    both.println("Processing Error \n Retrying...");
    delay(5000);
    display.clear();
    http.end();
    a++;
    b = 1;
    return;
  }

  else if (httpCode == 500 || httpCode == 503) {
    both.println("Server Under Maintenance \n Retrying in 5 min...");
    http.end();
    a++;
    b = 0;
    c = 0;
    startTime = millis();
    waitLength = 300000UL;
    waiting = true;
    return;

  } else if (httpCode == 403 || httpCode == 401) {
    both.println("Server Refusing User \n Shutting Down...");
    http.end();
    while (1) delay(1);

  } else if (httpCode == 413) {
    both.println("Sending Data \n too Large \n Code needs Repair");
    http.end();
    while (1) delay(1);
  }

  else if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    both.println("Checking Server \n for Confirmation...");
    // file found at server

    if (httpCode <= 299 && httpCode >= 200) {
      both.println("Data Verified! \n Continuing...");
      http.end();
      a = 0;
      b = 0;
      c = 0;
      return;
    }

    else {
      both.println("Issue with Server \n Communication \n Retrying...");
      b = 1;
      a++;
      http.end();
    }
  }
}


void loop() {
  heltec_loop();
  if (millis() - startTime >= waitLength && waiting == true) {
    waiting = false;
    sendServer();
  }

  if (c == 1) {
    delay(1000);
    wifiSetup();
    delay(1000);
    sendServer();
  }
  if (b == 1) {
    delay(1000);
    sendServer();
  }

  if (packetReady) {
    packetReady = false;
    int state = radio.readData(data);
    both.println("Packet Received \n Processing...");
    delay(500);
    radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
    if (state == RADIOLIB_ERR_NONE) {
      delay(500);
      both.println(data);
      delay(4500);
      display.clear();
      if (waiting == false && b == 0 && c == 0){
        both.println("Sending to Server...");
        sendServer();
    } else {
      both.println("Retry Pending \n Data Queued");
    }
  }
}
}
