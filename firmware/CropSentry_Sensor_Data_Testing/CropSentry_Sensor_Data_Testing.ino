#include <heltec_unofficial.h>
#include <Wire.h>
#include <Adafruit_BME680.h>
#include <Adafruit_SHT4x.h>
Adafruit_SHT4x sht4;

Adafruit_BME680 bme(&Wire1);

#define PAUSE 0             // MS between packet transmission, already computed in void loop() - Will send when button pressed
#define FREQUENCY 905.2     // 905.2 MHZ, legal band for ISM - POWER MUST BE UNDER 30 dBm - ANTENNA UNDER 6 dBi
#define BANDWIDTH 500.0     // 500 kHz, MAKE SURE Serial BOARDS ARE SET TO THIS
#define SPREADING_FACTOR 9  // Controls how fast or slow board sends, longer should mean slower
#define TRANSMIT_POWER 0    // Boards are close together for testing - don't want to send too much transmit power

#define SM_A 2  // Soil Moisture Analog

// Setting failsafe vars, if sensor repsonds with unrealistic numbers, terminate whole send
unsigned long lastRun = 0;
unsigned long waitTime = 0;
int iter = 0;       // Variable used to determine how many times to repeat loop after fail before device stops
int radioiter = 0;  // Used to determine whether the sensors or radio is failing
volatile bool rxFlag = false;

struct SensorPacket {
  uint64_t nodeid;  // Uses board SN for variety

  int sequence;
  int soilm;

  float temp1;

  float humid1;
  float humid2;
  float gas;

  // Needs verification, may count on server
  int samplec;

  // Needs sensor, looking at SGP41, but it might not come in the mail until October
  float temp2;
  float voc1;
  float voc2;
};

SensorPacket SensorData;



void setup() {
  heltec_setup();
  heltec_ve(true);  // Uses on-board VEXT pin for the displays 3V3 power source
  delay(100);
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);
  delay(50);
  digitalWrite(21, HIGH);
  delay(50);  // Sets pin 21 (display controller) LOW and HIGH for power configuration

  display.init();
  display.flipScreenVertically();
  SensorData.nodeid = (uint64_t)(ESP.getEfuseMac() >> 16);

  // limits the amount of numbers shown on the boards SN as a variable, keeps it usable for a finite and autonomous identification system

  display.clear();
  display.drawString(0, 0, "Starting Module");
  display.display();
  delay(5000);
  heltec_led(50);           // LED brightness, 50% is enough
  Serial.begin(115200);     // 115200 baud, 8 SensorData bits, N for no parity, 1 stop bit, on the SDA and SCL pins
  pinMode(SM_A, INPUT);     // Setting Soil Moisture Probe to an input
  SensorData.sequence = 0;  // Setting Sequence to 0
  Wire1.begin(41, 42);

  // put your setup code here, to run once:
  // Wire1.begin(8, SDA, SCL);       // Sets general I2C address to 8
  // Wire1.onRequest(requestEvent);  // When told to request SensorData, run a function, such as void requestEvent()
  // Wire1.write((uint8_t*)&data, sizeof(data));
  display.clear();
  display.drawString(0, 0, "Device Initializing");
  display.display();
  delay(5000);
  Serial.println("Device Initializing:");

  bme.begin();
  bme.setHumidityOversampling(BME680_OS_16X);  // Uses library example standard, 16x for accuracy
  bme.setGasHeater(320, 150);                  // 320*C for 150 ms microscopic
  // Chose to not use temp in BME due to microscopic heat affecting accuracy

  sht4.begin(&Wire1);
  sht4.setPrecision(SHT4X_HIGH_PRECISION);  // Used library example for high precison
  sht4.setHeater(SHT4X_NO_HEATER);  // Sets microscopic heater in SHT4X module to high heat for particles

  display.clear();
  display.drawString(0, 0, "Device Connected!");
  display.display();
  delay(5000);
  Serial.println("Device Connected!");
}


void bmefunc() {
  float gasSum = 0;
  float humidSum = 0;
  int kept = 0;
  for (int i = 0; i < 10; i++) {
    bme.performReading();
    if (i > 4) {
      gasSum += bme.gas_resistance / 1000.0;  // Converts to KOhms for more compressable readings
      humidSum += bme.humidity;
      kept++;
    }
    delay(2000);
  }
  SensorData.humid1 = humidSum / kept;
  SensorData.gas = gasSum / kept;  // Finds the average
}


void sht45func() {
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);
  SensorData.temp1 = temp.temperature;
  SensorData.humid2 = humidity.relative_humidity;
}

void SMP() {  // Soil Moisture Probe
  SensorData.soilm = analogRead(SM_A);
}

void loop() {
  heltec_loop();                              // Checks for button press
  if (millis() - lastRun < waitTime) return;  // Not enough time has passed
  lastRun = millis();                         // Enough time has passed, runs the loop
  heltec_led(50);
  Serial.println("Running Sensors ETA 2 min:");
  display.clear();
  display.drawString(0, 0, "Running Sensors ETA 2 min:");
  display.display();
  delay(5000);


  bmefunc();

  sht45func();

  SMP();
  Serial.println(String(SensorData.sequence) + "|\n" + String(SensorData.temp1) + "|" + String(SensorData.humid1) + "|\n" + String(SensorData.humid2) + "|" + String(SensorData.soilm) + "|\n" + String(SensorData.gas));  // Defining the packet that will be sent over LoRa added in \n

}