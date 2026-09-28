#include <Wire.h>
#include <Adafruit_BME680.h>
#include <Adafruit_SHT4x.h>
Adafruit_SHT4x sht4;

Adafruit_BME680 bme(&Wire);

#define SM_A 1  // Soil Moisture Analog

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
int iter = 0;  // Variable used to determine how many times to repeat loop after fail before device stops

struct SensorPacket {
  uint64_t nodeid;

  int sequence;

  float temp1;

  float humid1;
  float humid2;
  float soilm;
  float gas;

  bool validity;

  // Needs verification, may count on server
  int samplec;

  // Needs sensor, looking at SGP41, but it might not come in the mail until October
  float temp2;
  float voc1;
  float voc2;
};

SensorPacket SensorData;



void setup() {
  // put your setup code here, to run once:
  // Wire1.begin(8, SDA, SCL);       // Sets general I2C address to 8
  // Wire1.onRequest(requestEvent);  // When told to request SensorData, run a function, such as void requestEvent()
  // Wire1.write((uint8_t*)&data, sizeof(data));
  Serial0.begin(115200, SERIAL_8N1, SDA, SCL);  // 115200 baud, 8 SensorData bits, N for no parity, 1 stop bit, on the SDA and SCL pins
  pinMode(SM_A, INPUT);                         // Setting Soil Moisture Probe to an input
  SensorData.sequence = 0;                      // Setting Sequence to 0
  SensorData.nodeid = ESP.getEfuseMac();

  if (!bme.begin()) {  // Checks for if BME is working
    Serial0.println("No BME688 Connected, restart code");
    while (1) delay(1);  // Kills code, prevents processor from overworking this loop
  }
  bme.setHumidityOversampling(BME680_OS_16X);  // Uses library example standard, 16x for accuracy
  bme.setGasHeater(320, 150);                  // 320*C for 150 ms microscopic
  // Chose to not use temp in BME due to microscopic heat affecting accuracy

  if (!sht4.begin()) {  // Checks for if SHT45 is working
    Serial0.println("Couldn't find SHT4x");
    while (1) delay(1);  // Kills code, prevents processor from overworking this loop
  }

  sht4.setPrecision(SHT4X_HIGH_PRECISION);  // Used library example for high precison

  case SHT4X_HIGH_HEATER_100MS:{  // Sets humid sensor in SHT45 to high power, case always requires colon, so loop is necessary
  }

}


  void bmefunc() {
    float gasSum = 0;
    float humidSum = 0;
    int kept = 0;
    for (int i = 0; i < 10; i++) {
      if (!bme.performReading()) {
        c = 1;
        Serial0.println("Failed to perform reading :(");
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
      Serial0.println("SHT45 Cannot Read");
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
    if (millis() - lastRun < waitTime) return;
    lastRun = millis();
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


    if (a == 1.0 || b == 1.0 || c == 1.0 || d == 1.0 || x == 1.0 || y == 1.0 || z == 1.0) {  // If any value is equal to 1, run failsafe
      iter = iter + 1;

      if (iter == 3) {  // If loop ran through 3 times with fail, close code
        Serial0.println("Sensor Damaged, Not Receiving Info After 3 Attempts. Please Reapir and Reset");
        while (1) delay(1);  // Stops all code
      }
      waitTime = 0;
      return;
    }
    SensorData.sequence++;
    iter = 0;
    waitTime = 1542857UL;  // 25 minute delay for next round
  }