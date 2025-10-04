/*
#######################################################
CAVEduino datalogger
GREATER HOUSTON GROTTO - ALBERTO GAUDIO 2025
FOR TEMP, HUM, PRESSURE LOGGING. BASED ON XIAO ESP32C6
#######################################################
*/

#include "FS.h"
#include "SD.h"
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "RTClib.h"
#include "esp32-hal-cpu.h"
#include "driver/periph_ctrl.h"
#include "driver/rtc_io.h"
RTC_DS3231 rtc;

// the pin that is connected to SQW of the RTC
#define USE_EXT0_WAKEUP 1
#define WAKEUP_GPIO GPIO_NUM_0
#define NUOVO_INTERRUPT 0
#define BUTTON_PIN_BITMASK (1ULL << GPIO_NUM_0) // GPIO 0 bitmask for ext1

// Define SD card connection on SDI pins
#define SD_MOSI     18
#define SD_MISO     20
#define SD_SCLK     19
#define SD_CS       2

//CHANGE HERE THE NAME OF THE FILE FOR EACH DATALOGGER
#define nomefile "/data_logger_Nr1.csv"
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

  //LOWERING CPU FREQUENCY TO SAVE POWER - 10 IS THE MINIMUM
  setCpuFrequencyMhz(10);

  //ACTIVATE THE FUNCTION OF THE BUILT-IN YELLOW LED
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("Setup start");
  Serial.flush();

  //INIZIALIZING RTC
  if (! rtc.begin())
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

  //SHOW INFO REGARDING THE PREVIOUS ALARM
  DateTime alarm1 = rtc.getAlarm1();
  Serial.print("Minutaggio allarme precedente: ");
  Serial.println(alarm1.minute());
  Serial.flush();

  //#####################################ALARM EVERY 3 MINUTES, FOR TESTING ONLY - COMMENT THIS SECTION FOR OPERATIONS
  DateTime currenttime = rtc.now();
  int nuovotempo = (currenttime.minute());//, DEC);
  nuovotempo +=3;
  if (nuovotempo > 59) {nuovotempo -= 60;}
  
  /////##################################ALARM EVERY 30 MINUTES - UNCOMMENT THIS ONE FOR OPERATIONS
  //int nuovotempo = alarm1.minute();
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


  //#########################################################################################
  //UNCOMMENT THE FOLLOWING LINE FOR RTC SET DATE AND TIME. TO BE DONE ONLY ONCE.
  //rtc.adjust(DateTime(F(__DATE__),F(__TIME__))); //#######################################################
  // ###########################################################################################3

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
  if (!bme.begin(0x77, &Wire))
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

  //BLINK ONE THE YELLOW LED TO SAY ALL IS GOOD!
  blink_pattern("01");

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
                    
  
  ///START SETTING UP THE SD CARD USING SPI
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  
  //INITIALIZE SDI DEVICE 
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  //INITIALIZE AND MOUNT SD CARD
  if (!SD.begin(SD_CS)) 
  {
    Serial.println("SD Card MOUNT FAIL");
    for (int i = 0; i < 3; i++) {
      blink_pattern("010101010101");
      delay(3000);
    }
    Serial.flush();
    while (1) delay(10);
  } 
  else 
  {
    Serial.println("SD Card MOUNT SUCCESS");
    Serial.println("");
    Serial.flush();
  }
   
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

  delay(5000); //DELAY ADDED TO WARM UP BME280 SENSOR

  //SETUP BME280 SENSOR VARIABLES
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);
  
  //SHOWING THE TIMESTAMP ON TERMINAL
  DateTime now = rtc.now();

  Serial.print(now.year(), DEC);
  Serial.print("-");
  Serial.print(now.month(), DEC);
  Serial.print("-");
  Serial.print(now.day(), DEC);
  Serial.print("-");
  Serial.print("");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);
  Serial.flush();

  //WRITING ALL THE DATA IN THE LOGGING FILE
  myFile = SD.open(nomefile, FILE_APPEND); //FILE_WRITE SOVRASCRIVE, FILE_APPEND FUNZIONA CON ESP32
  if (myFile) 
  {
    Serial.print("Writing data to SD card...");

    myFile.print(now.year());
    myFile.print("/");
    myFile.print(now.month());
    myFile.print("/");
    myFile.print(now.day());
    myFile.print(" ");
    myFile.print(now.hour());
    myFile.print(":");
    myFile.print(now.minute());
    myFile.print(":");
    myFile.print(now.second());
    myFile.print(",");
    myFile.print(temp_event.temperature);
    myFile.print(",");
    myFile.print(humidity_event.relative_humidity);
    myFile.print(",");
    myFile.println(pressure_event.pressure);

  // close the file:
    myFile.close();
    Serial.println("...done.");
    Serial.flush();
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
  btStop();
  SD.end();
  SPI.end();

  periph_module_disable(PERIPH_SPI2_MODULE);
  digitalWrite(SD_MISO, LOW);
  digitalWrite(SD_MOSI, LOW);
  digitalWrite(SD_SCLK, LOW);
  Wire.end();

  //SLEEP MODE SETUP SPECIFIC FOR ESP32C6 - CHAGE THIS WITH OTHER ESP32 BOARDS
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_LOW);

  ///SETTING SD CARD PINS FOR THE SLEEP
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, LOW);

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
