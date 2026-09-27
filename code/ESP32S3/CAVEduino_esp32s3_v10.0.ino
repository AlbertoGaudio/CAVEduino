/*
#######################################################
CAVEduino datalogger
GREATER HOUSTON GROTTO - ALBERTO GAUDIO 2026
FOR TEMP, HUM, PRESSURE LOGGING. BASED ON ADAFRUIT ESP32S3
#######################################################
*/

#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "RTClib.h"
#include "esp32-hal-cpu.h"
#include "driver/periph_ctrl.h"
#include "driver/rtc_io.h"
#include "WiFi.h"
// INTERNAL PULLUP RATHER THAN EXTERNAL 10K OHM RESISTOR FROM 3V TO THE WAKEUP PIN - DO NOT USE IF YOU HAVE A PHYSICAL 10K RESISTOR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
#include "driver/rtc_io.h"

RTC_DS3231 rtc;

// the pin that is connected to SQW of the RTC
#define USE_EXT0_WAKEUP 1
#define WAKEUP_GPIO GPIO_NUM_18
#define NUOVO_INTERRUPT 0
#define BUTTON_PIN_BITMASK (1ULL << GPIO_NUM_0) // GPIO 0 bitmask for ext1

// Define SD card connection on SDI pins
#define SD_MOSI     35
#define SD_MISO     37
#define SD_SCLK     36
#define SD_CS       TX

//CHANGE HERE THE NAME OF THE FILE FOR EACH DATALOGGER
#define nomefile "/Logger05.csv"
File myFile;

//BME280 SETTING VARIABLES
Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();

//THIS DEFINES HOW LONG THE LED BLINK IS 
static const uint16_t BLINK_PERIOD = 100;

void setup()
{

  Serial.begin(115200);
  delay(10);

  // INTERNAL PULLUP RATHER THAN EXTERNA 10K OHM RESISTOR FROM 3V TO THE WAKEUP PIN - DO NOT USE IF YOU HAVE A PHYSICAL 10K RESISTOR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
  rtc_gpio_pullup_en(WAKEUP_GPIO);
  rtc_gpio_pulldown_dis(WAKEUP_GPIO);

  //LOWERING CPU FREQUENCY TO SAVE POWER - 10 IS THE MINIMUM
  //setCpuFrequencyMhz(10);

  //ACTIVATE THE FUNCTION OF THE BUILT-IN YELLOW LED
  pinMode(LED_BUILTIN, OUTPUT);
  //pinMode(SD_CS, INPUT_PULLUP);
  
  Serial.println("Setup start");
  Serial.flush();

  //INIZIALIZING RTC
  if (! rtc.begin(&Wire1))
  {
    Serial.println("Couldn't find RTC - cass!");
    Serial.flush();
    for (int i = 0; i < 3; i++) {
      blink_pattern("010101");
      delay(3000);
    }
    while (1) delay(10);
  }

  //DISABLE THE 32K OSCILLATOR
  rtc.disable32K();
  
  //SETTING UP PINS FOR THE WAKEUP FUNCTION
  pinMode(WAKEUP_GPIO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WAKEUP_GPIO), onAlarm, FALLING);
  Wire1.begin();

  //SHOW INFO REGARDING THE PREVIOUS ALARM
  DateTime alarm1 = rtc.getAlarm1();
  Serial.print("Minutaggio allarme precedente: ");
  Serial.println(alarm1.minute());
  Serial.flush();

  //#####################################ALARM EVERY 3 MINUTES, FOR TESTING ONLY - COMMENT THIS SECTION FOR OPERATIONS
  DateTime currenttime = rtc.now();
  int nuovotempo = (currenttime.minute());//, DEC);
  nuovotempo +=1;
  if (nuovotempo > 59) {nuovotempo -= 60;}
  
  /////##################################ALARM EVERY 30 MINUTES - UNCOMMENT THIS ONE FOR OPERATIONS
  // int nuovotempo = alarm1.minute();
  // if (nuovotempo == 29) { nuovotempo = 59; }
  // else{ nuovotempo = 29; }

  Serial.print("Nuovo allarme:");
  Serial.println(nuovotempo);
  Serial.flush();

  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  rtc.writeSqwPinMode(DS3231_OFF);

  //REMOVE PREVIOUS ALARMS
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);

  // SCHEDULE THE NEW ALARM
  if (!rtc.setAlarm1(DateTime(0,0,0,0,nuovotempo,0),DS3231_A1_Minute)) {  // this mode triggers the alarm when the minutes match
    Serial.println("Error, alarm wasn't set!");
    Serial.flush();
  } else {
    Serial.println("Alarm will happen at specified time");
    Serial.flush();
  }

  //BME280 SENSOR INITIALIZING
  if (!bme.begin(0x77, &Wire1))
  {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    for (int i = 0; i < 3; i++) {
      blink_pattern("01010101010101010101");
      delay(3000);
    }
    Serial.flush();
    while (1) delay(10);
  }
  else
  {
    Serial.println("BME280 sensors seem to be working!");
    Serial.println("");
    Serial.flush();
  }


  ///#### SETTINGS SUGGESTED FOR WEATHER MONITORING BY BOSH BME280 OFFICIAL MANUAL - COMMENT THIS SECTION FOR SMOOTHER DATA
  Serial.println("-- Weather Station Scenario --");
  Serial.println("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
  Serial.println("filter off");
  Serial.flush();
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF );
  
  delay(100);  
  
  //INITIALIZE SDI DEVICE
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  //SPI.begin(SD_SCLK, SD_MISO, SD_MOSI);

  delay(100);

  //INITIALIZE AND MOUNT SD CARD
  if (!SD.begin(SD_CS, SPI)) 
  {
    Serial.println("SD Card MOUNT FAIL veloce");
    for (int i = 0; i < 3; i++)
    {
      blink_pattern("010101010101");
      delay(3000);
    }

    Serial.flush();
  } 
  else 
  {
    Serial.println("SD Card MOUNT SUCCESS");
    Serial.println("");
    Serial.flush();
  }

  //BLINK ONE THE YELLOW LED TO SAY ALL IS GOOD!
  blink_pattern("01");
   
  //WRITE THE LOGGING FILE HEADER IF FILE DOES NOT EXISTS
  if (SD.exists(nomefile)) 
    {
      Serial.println("....data file already exists logging resumed...");
      Serial.println("");
      Serial.flush();
    } 
    else 
    {
      myFile = SD.open(nomefile, FILE_APPEND); //FILE_WRITE SOVRASCRIVE, FILE_APPEND FUNZIONA CON ESP32
      myFile.println("Timestamp, temp(*C),humidity(%),pressure(hPa)");
      myFile.close();
      Serial.println("New logging file created");
      Serial.println("File Header Complete");
      Serial.println("");
      Serial.flush();
    }  

  //INFO THAT THE SETUP IS COMPLETE   
  Serial.println("INFO: Setup complete. Everything seems to work !!!");
  Serial.println("");
  Serial.flush();

  delay(1000); //DELAY ADDED TO WARM UP BME280 SENSOR

  //SETUP BME280 SENSOR VARIABLES
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);
  
  //WRITING ALL THE DATA IN THE LOGGING FILE
  myFile = SD.open(nomefile, FILE_APPEND); //FILE_WRITE SOVRASCRIVE, FILE_APPEND FUNZIONA CON ESP32
  if (myFile) 
  {
    Serial.print("Writing data to SD card...");

    // Pre-allocate buffer for efficiency
    char buffer[150];
    
    DateTime now = rtc.now();
    
    // Format entire line at once
    int len = snprintf(buffer, sizeof(buffer), 
        "%d/%d/%d %d:%d:%d,%.2f,%.2f,%.2f",
        now.year(), now.month(), now.day(),
        now.hour(), now.minute(), now.second(),
        temp_event.temperature,
        humidity_event.relative_humidity,
        pressure_event.pressure);
    
    if (len > 0) {
        myFile.println(buffer);
    }

    delay(100);
    myFile.flush();
  
  // close the file:
    myFile.close();

  } 
  else 
  {
    // if the file didn't open, print an error.
    Serial.println("error opening data.csv");
    Serial.flush();
  }

  Serial.println("Going to sleep now");
  Serial.flush();

  // CLOSING SERVICES TO SAVE POWER DURING SLEEP
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  SD.end();
  SPI.end();
  //periph_module_disable(PERIPH_SPI_MODULE);
  //periph_module_disable(PERIPH_SPI2_MODULE);
  Wire.end();

  //SLEEP MODE WORKING for ESP32-S3 ONLY
  esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 0);

  Serial.flush();
  Serial.end();

  //ENTER IN DEEP SLEEP MODE
  esp_deep_sleep_start();

}

void loop() {
  // NOT NEED TO USE THIS LOOP BECAUSE OF THE SLEEP MODE
}

void onAlarm() {} //FAKE FUNTION FOR THE INTERRUPT PIN SETUP 

//YELLOW LED BLINKING FUNCTION
void blink_pattern(char pattern[])
{
  for (size_t index = 0; index < strlen(pattern); index ++)
  {
    switch (pattern[index]) {
      case '0':
        digitalWrite(LED_BUILTIN, LOW);
        delay(BLINK_PERIOD);
        break;

      case '1':
        digitalWrite(LED_BUILTIN, HIGH);
        delay(BLINK_PERIOD);
        break;          
    }
  }
}


