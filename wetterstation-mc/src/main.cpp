#include <Arduino.h>
#include <senseBoxIO.h>
#include "config.h"
#include <SPI.h>
#include <Wire.h>

#include <Adafruit_BMP280.h>
#include <Adafruit_HDC1000.h>
#include <Makerblog_TSL45315.h>
#include "SdsDustSensor.h"
#include <VEML6070.h>
#include <WiFi101.h>

#include "measurement.h"
#include "display.cpp"
#include "network.cpp"

#include "utils.cpp"

/*
    Überprüfe welche Sensoren und Module
    verbunden sind.
*/

#ifdef SSD1306_CONNECTED
WSDisplay *display;
#endif

#ifdef BMP280_CONNECTED
Adafruit_BMP280 bmp;
#define P0 1013.25
#endif

#ifdef HDC1080_CONNECTED
Adafruit_HDC1000 hdc = Adafruit_HDC1000();
#endif

#ifdef TSL45315_CONNECTED
Makerblog_TSL45315 tsl = Makerblog_TSL45315(TSL45315_TIME_M4);
#endif

#ifdef VEML6070_CONNECTED
VEML6070 veml;
#endif

#ifdef PM_CONNECTED
SdsDustSensor sds(Serial1); // passing HardwareSerial& as parameter
#endif

// TODO: Auslagern
const int BUTTON_PIN = 0;
volatile unsigned long switchTime = 0, bounceTime = 15;

unsigned long lastMillis1;

// Klasse zum übertragen der Messungen an openSenseMap
Network network(SERVER_ADDRESS);

// Klasse welche alle Messungen sammelt und über Zeiger
// mit anderen Klassen geteilt wird. (Display, Network)
Measurment data;

/**
 * 
 * Überprüfe die I2C Schnittstelle nach verfügbaren Sensoren
 * 
 **/
void checkI2CSensors()
{
  byte error;
  int nDevices = 0;
  byte sensorAddr[] = {41, 56, 57, 64, 118};
  DEBUG(F("\nScanning..."));
  for (unsigned int i = 0; i < sizeof(sensorAddr); i++)
  {
    Wire.beginTransmission(sensorAddr[i]);
    error = Wire.endTransmission();
    if (error == 0)
    {
      nDevices++;
      switch (sensorAddr[i])
      {
      case 0x29:
        DEBUG(F("TSL45315 found."));
        break;
      case 0x38: // &0x39
        DEBUG(F("VEML6070 found."));
        break;
      case 0x40:
        DEBUG(F("HDC1080 found."));
        break;
      case 0x76:
        DEBUG(F("BMP280 found."));
        break;
      }
    }
    else if (error == 4)
    {
      DEBUG2(F("Unknown error at address 0x"));
      if (sensorAddr[i] < 16)
        DEBUG2(F("0"));
      DEBUG_ARGS(sensorAddr[i], HEX);
    }
  }
  if (nDevices == 0)
  {
    DEBUG(F("No I2C devices found.\nCheck cable connections and press Reset."));
    while (true)
      ;
  }
  else
  {
    DEBUG2(nDevices);
    DEBUG(F(" sensors found.\n"));
  }
}

/**
 * 
 * Vorbereitung der Sensor Daten für die openSenseMap.
 * Wenn ein Sensor verfügbar ist wird er der dem Webrequest
 * der Netzwerk Klasse hinzugefügt.
 * 
 * Diese Methode wird von der Netzwerk Klasse aufgerufen
 * und in der loop() Methode als Zeiger übergeben.
 * 
 **/
void prepostSensorData()
{
  DEBUG(F("[Prepostdata] has started"));
// BMP280
#ifdef BMP280_CONNECTED
#ifdef TEMPERATURE_ID
  network.addMeasurement(TEMPERATURE_ID, data.Temperature);
#endif
#ifdef PRESSURE_ID
  network.addMeasurement(PRESSURE_ID, data.Pressure);
#endif
#ifdef ALTITUDE_ID
  network.addMeasurement(ALTITUDE_ID, data.Altitude);
#endif
#endif

// HDC1080
#ifdef HDC1080_CONNECTED
#ifdef TEMPERATURE_ID
  network.addMeasurement(TEMPERATUR_ID, data.Temperature);
#endif
#ifdef HUMIDITY_ID
  network.addMeasurement(HUMIDITY_ID, data.Humidity);
#endif
#endif

// TSL45315
#ifdef TSL45315_CONNECTED
  network.addMeasurement(ILLUMINANCE_ID, data.Lux);
#endif

// VEML6070
#ifdef VEML6070_CONNECTED
  network.addMeasurement(UV_RADIATION_ID, data.UV);
#endif

//SDS011
#ifdef PM_CONNECTED
  network.addMeasurement(PM_PM25_ID, data.pm25);
  network.addMeasurement(PM_PM10_ID, data.pm10);
#endif

// WINDRAD
#ifdef WINDRAD_CONNECTED
  network.addMeasurement(WINDRAD_DIRECTION_ID, data.Winddirection);
  network.addMeasurement(WINDRAD_SPEED_ID, data.Windspeed);
#endif

  DEBUG(F("[Prepostdata] was completed..."));
}

/**
 * 
 * Aktuallisert die Sensordaten
 * wird in der loop() Methode aufgerufen.
 * 
 **/
void updateSensorData()
{
// BMP280
#ifdef BMP280_CONNECTED
  data.Temperature = bmp.readTemperature();
  data.Pressure = bmp.readPressure() / 100;
  data.Altitute = bmp.readAltitude(1013.25);
#endif

// HDC1080
#ifdef HDC1080_CONNECTED
  data.Temperature = hdc.readTemperature();
  delay(200);
  data.Humidity = hdc.readHumidity();
#endif

// TSL45315
#ifdef TSL45315_CONNECTED
  data.Lux = tsl.readLux();
#endif

// VEML6070
#ifdef VEML6070_CONNECTED
  data.UV = veml.getUV();
#endif

//SDS011
#ifdef PM_CONNECTED
  sds.wakeup();
  PmResult pm = sds.queryPm();
  if (pm.isOk()) {
    Serial.print("PM2.5 = ");
    Serial.print(pm.pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm.pm10);

    // if you want to just print the measured values, you can use toString() method as well
    Serial.println(pm.toString());
  } else {
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }

  WorkingStateResult state = sds.sleep();
  if (state.isWorking()) {
    Serial.println("Problem with sleeping the sensor.");
  } else {
    Serial.println("Sensor is sleeping");
    delay(60000); // wait 1 minute
  }
#endif

#ifdef WINDRAD_CONNECTED
  IPAddress addrx(PM_IPADDR);
  network.getValuesFromUrl(&addrx, 80);
  data.Winddirection = network.windradDirection;
  data.Windspeed = network.windradSpeed;
#ifdef PM_CONNECTED
  data.pm25 = network.pm25;
  data.pm10 = network.pm10;
#endif
#endif
}

/**
 * 
 * Initialisiere Sensoren
 * Wenn ein Sensor verfügbar ist und in der Konfiguration
 * aktiviert ist wird er hier für die Messungen vorbereitet.
 * 
 * Diese Methode wird einmalig in der setup() Methode aufgerufen.
 * 
 **/
void initSensors()
{
  DEBUG(F("Initializing sensors..."));

#ifdef HDC1080_CONNECTED
  hdc.begin();
#endif
#ifdef BMP280_CONNECTED
  bmp.begin(0x76);
#endif
#ifdef VEML6070_CONNECTED
  veml.begin();
  delay(500);
#endif
#ifdef TSL45315_CONNECTED
  tsl.begin();
#endif
#ifdef PM_CONNECTED
  //PM_UART.begin(9600);
  sds.begin();
  Serial.println(sds.queryFirmwareVersion().toString()); // prints firmware version
  Serial.println(sds.setQueryReportingMode().toString());
#endif

  DEBUG(F("Initializing sensors done!"));
  delay(3000);
}

/**
 * 
 * Interrupt Methode, diese Methode hört auf den Pin 0 (Switch) und unterbricht die
 * Hauptprogramm Routine für die Abfrage des Tasters.
 * Es wird auf die nächste Seite des Displays gewechselt. 
 * 
 **/
void switchPress()
{
  if ((millis() - switchTime) > bounceTime)
  {
#ifdef SSD1306_CONNECTED
    display->nextPage();
    switchTime = millis();
#endif
  }
}

/**
 * 
 * Hauptroutine, zu jeweiligen Zeiten wird eine Aktion der verschiedenen
 * Programmteile ausgeführt.
 * 
 **/
void loop()
{
  // Routine für die Aktualisierung der Sensor Daten beginnt.
  // TODO: Auslagerung der SensorDaten Methode in eine eigene Klasse.
  if ((millis() - lastMillis1) >= 10e3)
  {
    updateSensorData();
    //TODO: Display (Fehler, Warnungen) behandeln.
    lastMillis1 = millis();
  }

  // Netzwerk Klasse übernimmt aufgaben,
  // zuvor wird die 'prepostSensorData' Methode aufgerufen.
  network.networkHandle(prepostSensorData);

  // Display Klasse übernimmt ihre Aufgaben.
  // Anzeigen von verschiedenen Seiten.
  display->handleDisplay();
}

void setup()
{
#ifdef ENABLE_DEBUG
  Serial.begin(9600);
#endif
  delay(5000);

  DEBUG2(F("xbee1 spi enable..."));
  senseBoxIO.SPIselectXB1();
  DEBUG(F("done"));
  senseBoxIO.powerXB1(false);
  delay(200);
  DEBUG2(F("xbee1 power on..."));
  senseBoxIO.powerXB1(true);
  DEBUG(F("done"));
  senseBoxIO.powerI2C(false);
  delay(200);
  senseBoxIO.powerI2C(true);
  delay(2000);

#ifdef ENABLE_DEBUG
  Wire.begin();
  checkI2CSensors();
#endif

// Initialisiere Display
#ifdef SSD1306_CONNECTED
  display = new WSDisplay(&data);
#endif

  // Initialisiere Netzwerk
  network.initialize(NET_SSID, NET_PASS);

  // Setzte auf Pin 0 (Switch Button) ein interrupt
  pinMode(0, INPUT);
  attachInterrupt(0, switchPress, LOW);

  initSensors();
  updateSensorData();

  lastMillis1 = millis();

// Aktualisiere Display Zeiten um aktiv zu bleiben
#ifdef SSD1306_CONNECTED
  display->adjustmentMillis();
#endif
}