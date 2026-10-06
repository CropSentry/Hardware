#include <heltec_unofficial.h>
#include <Wire.h>
#include <Adafruit_BME680.h>
#include <Adafruit_SHT4x.h>
Adafruit_SHT4x sht4;

Adafruit_BME680 bme(&Wire);

#define PAUSE 0             // MS between packet transmission, already computed in void loop() - Will send when button pressed
#define FREQUENCY 905.2     // 905.2 MHZ, legal band for ISM - POWER MUST BE UNDER 30 dBm - ANTENNA UNDER 6 dBi
#define BANDWIDTH 500.0      // 500 kHz, MAKE SURE Serial BOARDS ARE SET TO THIS
#define SPREADING_FACTOR 9  // Controls how fast or slow board sends, longer should mean slower
#define TRANSMIT_POWER 0    // Boards are close together for testing - don't want to send too much transmit power

#define SM_A 2  // Soil Moisture Analog

int a = 0;  // Soil Moisture Sensor
int b = 0;  // Humid 2 Sensor
int c = 0;  // BME688 Reading Check
int d = 0;  // SHT45 Reading Check
int x = 0;  // Gas Sensor
int y = 0;  // Humid 1 Sensor
int z = 0;  // Temperature 1 Sensor
// Setting failsafe vars, if sensor repsonds with unrealistic numbers, terminate whole send
unsigned long lastRun = 0;
unsigned long waitTime = 0;
int iter = 0;       // Variable used to determine how many times to repeat loop after fail before device stops
int radioiter = 0;  // Used to determine whether the sensors or radio is failing
volatile bool rxFlag = false;

struct SensorPacket {
  uint64_t nodeid;  // Uses board SN for variety

  int sequence;

  float temp1;

  float humid1;
  float humid2;
  float soilm;
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

  RADIOLIB_OR_HALT(radio.begin());
  // Set the callback function for sending packets
  radio.startReceive();
  // Set radio parameters

  display.clear();
  display.drawString(0, 0, "Frequency: " + String(FREQUENCY, 2) + " MHz");
  display.display();
  delay(5000);
  Serial.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));

  display.clear();
  display.drawString(0, 0, "Bandwidth: " + String(BANDWIDTH, 1) + " kHz");
  display.display();
  delay(5000);
  Serial.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));

  Serial.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));

  Serial.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));

  // Used LoRa_rx_tx.ino library for this block

  display.clear();
  display.drawString(0, 0, "Radio Initialized");
  display.display();
  delay(5000);
  Serial.println("Radio Initialized");
  // put your setup code here, to run once:
  // Wire1.begin(8, SDA, SCL);       // Sets general I2C address to 8
  // Wire1.onRequest(requestEvent);  // When told to request SensorData, run a function, such as void requestEvent()
  // Wire1.write((uint8_t*)&data, sizeof(data));
  display.clear();
  display.drawString(0, 0, "Device Initializing");
  display.display();
  delay(5000);
  Serial.println("Device Initializing:");

  if (!bme.begin()) {  // Checks for if BME is working
    display.clear();
    display.drawString(0, 0, "No BME688 Connected, \n restart code");
    display.display();
    delay(5000);
    Serial.println("No BME688 Connected, restart code");
    while (1) delay(1);  // Kills code, prevents processor from overworking this loop
  }
  bme.setHumidityOversampling(BME680_OS_16X);  // Uses library example standard, 16x for accuracy
  bme.setGasHeater(320, 150);                  // 320*C for 150 ms microscopic
  // Chose to not use temp in BME due to microscopic heat affecting accuracy

  if (!sht4.begin()) {  // Checks for if SHT45 is working
    display.clear();
    display.drawString(0, 0, "Couldn't find SHT4x");
    display.display();
    delay(5000);
    Serial.println("Couldn't find SHT4x");
    while (1) delay(1);  // Kills code, prevents processor from overworking this loop
  }

  sht4.setPrecision(SHT4X_HIGH_PRECISION);  // Used library example for high precison
  sht4.setHeater(SHT4X_HIGH_HEATER_100MS);  // Sets microscopic heater in SHT4X module to high heat for particles

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
    if (!bme.performReading()) {
      c = 1;
      display.clear();
      display.drawString(0, 0, "Failed to perform \n reading");
      display.display();
      delay(5000);

      Serial.println("Failed to perform reading");
      return;
    }
    if (i > 4) {
      gasSum += bme.gas_resistance / 1000.0;  // Converts to KOhms for more compressable readings
      humidSum += bme.humidity;
      kept++;
    }
    delay(2000);
  }
  SensorData.humid1 = humidSum / kept;
  SensorData.gas = gasSum / kept;  // Finds the average

  if ((SensorData.gas) < 1 || (SensorData.gas > 2000)) {  // Sets var to 1, checked later in the loop for failsafe
    x = 1;
    return;
  }
  if ((SensorData.humid1) < 0.1 || (SensorData.humid1 > 100.0)) {
    y = 1;
    return;
  }
}


void sht45func() {
  sensors_event_t humidity, temp;  // Creates two new variables
  if (!sht4.getEvent(&humidity, &temp)) {
    display.clear();
    display.drawString(0, 0, "SHT45 Cannot Read");
    display.display();
    delay(5000);
    Serial.println("SHT45 Cannot Read");
    d = 1;
    return;
  }
  SensorData.temp1 = temp.temperature;
  SensorData.humid2 = humidity.relative_humidity;

  if (SensorData.temp1 < -40.0 || (SensorData.temp1) > 125.0) {
    z = 1;
    return;
  }
  if (SensorData.humid2 < 0.0 || SensorData.humid2 > 100.0) {
    b = 1;
    return;
  }
}

void SMP() {  // Soil Moisture Probe
  SensorData.soilm = analogRead(SM_A);

  if (SensorData.soilm < 800 || SensorData.soilm > 3400) {
    a = 1;
    return;
  }
}

void loop() {
  heltec_loop();  // Checks for button press
  if (millis() - lastRun < waitTime) return; // Not enough time has passed
  lastRun = millis(); // Enough time has passed, runs the loop
  heltec_led(50);
  Serial.println("Running Sensors ETA 2 min:");
  display.clear();
  display.drawString(0, 0, "Running Sensors \n ETA 2 min:");
  display.display();
  delay(5000);
  a = 0;
  b = 0;
  c = 0;
  d = 0;
  x = 0;
  y = 0;
  z = 0;         // resets all values for flags to catch
  delay(10000);  // 10 seconds to prepare
  bmefunc();
  delay(40000);  // 40 seconds to collect data
  sht45func();
  delay(40000);  // 40 seconds to collect data
  SMP();
  delay(15000);  // 15 seconds to complete
  Serial.println("Data Collected! \n Proceeding with Radio");
  display.clear();
  display.drawString(0, 0, "Data Collected, \n Proceeding with Radio");
  display.display();
  delay(5000);


  if (a == 1.0 || b == 1.0 || c == 1.0 || d == 1.0 || x == 1.0 || y == 1.0 || z == 1.0) {  // If any value is equal to 1, run failsafe
    iter++;
    if (iter == 3) {                                                                       // If sensor loop ran through 3 times with fail, end code
      Serial.println("Sensor Damaged, Not Receiving Info After 3 Attempts. Please Repair and Reset");
      display.clear();
      display.drawString(0, 0, "Sensor Damaged, \n Not Receiving Info \n After 3 Attempts. \n Please Repair \n and Reset");
      display.display();
      delay(5000);
      heltec_led(0);
      while (1) delay(1);  // Stops all code
    }
    heltec_led(0);
    Serial.println("Sensors Failed, Retrying");
    display.clear();
    display.drawString(0, 0, "Sensors Failed, Retrying");
    display.display();
    delay(5000);
    waitTime = 0;
    return;
  }

  String lora_msg = String(SensorData.nodeid, HEX) + "|" + String(SensorData.sequence) + "|" + String(SensorData.temp1) + "|" + String(SensorData.humid1) + "|" + String(SensorData.humid2) + "|" + String(SensorData.soilm) + "|" + String(SensorData.gas);  // Defining the packet that will be sent over LoRa

  // Packet format: nodeid|sequence|temp1|humid1|humid2|soilm|gas

  if (radio.transmit(lora_msg)) {  // Would be !radio.transmit, however, sucess for this command returns a 0, which is also false, so we don't use !
    // Sends the message through an if statement
    display.clear();
    display.drawString(0, 0, "Radio Transmit \n Failed, Retrying");
    display.display();
    delay(10000);  // Lets everything have time to get ready to run the loop again and for the user to see the radio failed
    radioiter++;

    if (radioiter == 3) {  // If radio loop ran through 3 times with fail, end code
      Serial.println("Radio Damaged, Not Receiving Info After 3 Attempts. Please Repair and Reset");
      display.clear();
      display.drawString(0, 0, "Radio Damaged, \n Not Receiving Info \n After 3 Attempts. \n Please Repair \n and Reset");
      display.display();
      delay(5000);
      heltec_led(0);
      while (1) delay(1);  // Stops all code
    }   
    display.clear();
    display.drawString(0, 0, "Radio Transmit \n Failed, Retrying");
    display.display();
    delay(10000);  // Lets everything have time to get ready to run the loop again and for the user to see the radio failed
    waitTime = 0;
    return;

  } else {
    display.clear();
    display.drawString(0, 0, "Radio Transmit \n Sucessful!");
    display.display();
    delay(5000);
    iter = 0;
    radioiter = 0;
    SensorData.sequence++;  // Adds one to the Sequence variable inside the struct
  }
    heltec_led(0);             // Turns LED off
    waitTime = 1500000UL;   // 25 minute delay for next round
}