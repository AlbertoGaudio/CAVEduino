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

//RTC_DATA_ATTR int bootCount = 0; ///NON LO METTO PERCHE' MI FREEZZA TUTTE LE VARIABILI

// Define SD card connection CONTROLLARE BENE SE CORRISPONDE ALLA SCHEDA
#define SD_MOSI     18
#define SD_MISO     20
#define SD_SCLK     19
#define SD_CS       2

//CAMBIARE QUI IL NOME DEL FILE A SECONDA DEL DATALOGGER
#define nomefile "/data_logger_Nr1.csv"

//QUESTO SERVE POI PER APRIRE E SCRIVERE IL FILE
File myFile;

//BME280 setto le variabili
Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();

//TEMPO DEL BLINKING PER CAPIRE ERRORI
static const uint16_t BLINK_PERIOD = 100;


void setup()
{
  Serial.begin(115200);

  //QUESTO SALVA UN BOTTO DI CORRENTE!!! RIDUCO LA FREQUENZA DELLA CPU AL MINIMO!!!
  setCpuFrequencyMhz(10);

  //ATTIVO IL LED INTEGRATO PER SEGNALARE GLI ERRORI
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("Setup start");
  Serial.flush();

  //INIZIALIZZO RTC
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

  //we don't need the 32K Pin, so disable it
  rtc.disable32K();
  
  //PREPARO I PIN PER LA SVEGLIA
  pinMode(WAKEUP_GPIO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WAKEUP_GPIO), onAlarm, FALLING);

  //trucco per fare l'allarme ogni 3 min ######## FOR TESTING PURPOSE ONLY
  DateTime alarm1 = rtc.getAlarm1();
  Serial.print("Minutaggio allarme precedente: ");
  Serial.println(alarm1.minute());
  Serial.flush();

  DateTime cazzo = rtc.now();
  int nuovotempo = (cazzo.minute());//, DEC);
  nuovotempo +=3;
  if (nuovotempo > 59) {nuovotempo -= 60;}
  
  /////##################################ALLARME OGNI 30 MIN #### FOR OPERATIONS
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


  //#########################################################################################3
  //RTC DATE AND TIME. SOLO UNA VOLTA POI COMMENTARE. USARE L'ESEMPIO INSTEAD: PIU' VELOCE A SETTARE
  //rtc.adjust(DateTime(F(__DATE__),F(__TIME__))); //#######################################################
  // ###########################################################################################3

  rtc.disableAlarm(1); //QUESTO SERVE PER ABBASSARE L'INTERRUTTORRRE CHE E' STATO APERTO PER IL WAKE-UP NEL CICLO PRECEDENTE
  rtc.disableAlarm(2);

  // Schedule an alarm
  if (!rtc.setAlarm1(DateTime(0,0,0,0,nuovotempo,0),DS3231_A1_Minute)) {  // this mode triggers the alarm when the minutes match
    Serial.println("Error, alarm wasn't set!");
    Serial.flush();
  } else {
    Serial.println("Alarm will happen at specified time");
    Serial.flush();
  }

  //CONTROLLO SE BME280 FUNZIONA
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

  //QUI FACCIO UN SINGOLO BLINK PER DIRE CHE TUTTO FUNZIONA
  blink_pattern("01");

  ///####EXPERIMENTAL!!!!#### TENTATIVO DI RIDURRE CONSUMO COME DA MANUALE. GUARDARE ESEMPIO AVANZATO DELLA LIBRERIA
  ///######## TESTARE CON ALTRO SENSORE ED EVENTUALMENTE TOGLIERE SE TROPPO BALLERINO. 
  Serial.println("-- Weather Station Scenario --");
  Serial.println("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
  Serial.println("filter off");
  Serial.flush();
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF );
                    
  
  ///PROVO A SETTARE IL CS PIN DELLA SD CARD TO HIGH, QUESTO EVITA PROBLEMI A MONTARE LA SD CARD
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  
  //CONTROLLO CHE LA SD FUNZIA
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

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
   
  //PREPARO LA HEADER DEL FILE, NON USO "WRITE" MA "APPEND" PER NON CANCELLARE NESSUN DATO PRESENTE
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

  //CONCLUDO IL SETUP DICENDOLO    
  Serial.println("INFO: Setup complete. Everything seems to work !!!");
  Serial.println("");
  Serial.flush();

  delay(5000); //AGGIUNGO QUI UN DELAY PER FAR SCALDARE IL SENSORE

  //ASSEGNO LE VARIABILI DEI SENSORI LEGGO I SENSORI
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);
  
  //Adesso mi occupo del timestamp
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

  //APRO IL FILE E SCRIVO LA STRINGA  
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

  //delay(5000);
  Serial.println("Going to sleep now");
  Serial.flush();
  
  // TENTO DI SALVARE ENERGIA
  btStop();
  SD.end();
  SPI.end();

  periph_module_disable(PERIPH_SPI2_MODULE);
  digitalWrite(SD_MISO, LOW);
  digitalWrite(SD_MOSI, LOW);
  digitalWrite(SD_SCLK, LOW);
  Wire.end();

  //QUESTA E' LA FUNZIONE PER PREPARARE LA ESP32S3 ALLO SLEEP
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_LOW);

  ///PROVO A SETTARE IL CS PIN DELLA SD CARD TO HIGH PER EVITARE PROBLEMI A MONTARE LA SD CARD
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, LOW); //EXPERIMENTAL ERA SU HIGH E FUNZIONAVA (NON SEMPRE)

  Serial.flush();
  Serial.end();

  //ENTER IN DEEP SLEEP MODE
  esp_deep_sleep_start();

}

void loop() {
  // NOT NEED TO USE THIS LOOP BECAUSE OF THE SLEEP MODE
}

void onAlarm() {} //QUESTO E' SOLO PER FORNIRE UN FALSO ISR ALL'INTERRUPT

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
