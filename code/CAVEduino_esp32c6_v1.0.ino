# CAVEduino
A datalogger for cave environmental monitoring.
A free and open source project developped by Alberto Gaudio of the Greater Houston Grotto. 
## Introduction
CAVEduino is an open source project to build and operate a datalogger to monitor cave environment. The project is based on ESP32 controller and it is studied to be easly transported and deployed in a cave. The hardware selection was studied to fit the purpose containing the costs, make it simple to replace batteries and retreive data and simple use and handling in cave. The logger can record a data every 30 min for approximately 6 months. This guide is to build a basic datalogger for Temperature, Pressure and Relative Humidity, however the possibilities to add any sort of sensor are limitless.

CAVEduino has been designed by cavers for cavers, therefore one of the main focus was on streamlining the battery swap and data retreival: you don't need to bring a laptop in cave to download the data, batteries are easy to transport and LED flashing codes provide information on normal operations.

**Carefully read all the instruction below. You will build and operate the CAVEduino at your own risk.**

## Equipment
1. XIAO ESP32C6 board
2. BME280 sensor breakboard (Temperature, Pressure, Humidity) (I2C)
3. DS3231 RTC (Real TIme Clock) (I2C)
4. MicroSD card breakout board (3 volt) (SDI)
5. 1 x 18650 lithium rechargeable battery (3000 - 4000mA)
6. CR1220 12mm Diameter - 3V Lithium Coin Cell Battery (CR1220)
7. A 1GB MicroSD card
7. An electrical switch (you can also find battery holders with an integrated switch)
8. Solder Breadboard and dupont connectors, wires etc.. of every sort
9. A 6'' x 6'' plastic box waterproof (a waterproof small storage parts organizer box will do the job)
10. A PVC tee 1/2'' pipe for plumbing with the the single lateral end threaded
11. A PVC male adapter for electrical conduits
12. 4 x cable zip tie mounting base 

## Wiring
This guide is written for the **XIAO ESP32C6** only. Check the [manufacturer page for the Pinout diagram](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/#hardware-overview) of this and other boards.

The first stage suggested is using a solderless breadboard to familiarize and check the wiring. Then you can solder everything use a solder breadboard after testing goes well.

**Real Time Clock (RTC) - I2C:**

    RTC SCL ----> D5 (SCL)
    RTC  SDA ----> D4 (SDA)
    RTC VCC ---- 3v3 output
    RTC Ground ----> GND
    RTC SQW ----> D0
    Batteria a moneta sul retro della scheda RTC

**BME280 - I2C:**

    BME280 SCK ----> D5 (SCL)
    BME280 SDI ----> D4 (SDA)
    BME 3Vo or VIN ----> 3v3
    BME280 Ground ----> GND

**SD card - SDI:**

    SD MISO ----> D9
    SD MOSI ----> D10
    SD SCK ----> D8
    SD CS ----> D2

## Microcontroller Flashing and Testing phase
At this stage you want to test the basic assemblage of the hardware. To do so you will be powering the system using the USB port of your computer. Battery pack will be installed in a second stage. **Do not connect the battery and the USB at the same time.**

#### Arduino IDE
Install the [Arduino IDE](https://www.arduino.cc/en/software/?ps_full_site=1)  on your computer.

If you are using Linux you will have to add a file to be able to upload the code to the microcontroller:

	" INSERT THE USB RULE CODE IN HERE"

#### Arduino IDE Libraries
In the Arduino IDE install the following libraries:

- RTClib

- BME 280 Adafruit

#### Sensors Testing Code
Use the code in the "Example Folder" of the Arduino IDE. You will find example code for testing all the parts (RTC, SD, BME280).


####First Troubleshoot
If you have issues while running the code, check every component individually using the example that can be found in the Arduino IDE software. So you can fix one component at the time if you have trouble.

**MicroSD Card issues**: The MicroSD card must be formatted using FAT16 or FAT32 file system. Ususally it is already formatted when new. The MicroSD card should not be oversized, in general smaller MicroSD are faster and more reliable than the large ones. In my project I am using a 1GB MicroSD card, because it i hard to find smaller MicroSD on the market today.

## Little hacks to save power
The sensors often have an integrated LED mounted that are constantly on when the system is powered. This is cause of significant battery drain on the long time. Therefore, after having tested that everything works well, you have to get rid of those LEDs.

#### DS3231 LED

#### BME280 LED


##Power consumption
The system power consumption with this configuration should be approximately 0.2mA when in sleep mode, and it is approximately 10.5mA when logging the data. Taking a measurement every 30min this gives an average of 0.4mA. Therefore, in order to log for 6 months, a battery with minimum 1800mA/h (before falling below 3v) is needed.   **Please note that actual performances of the batteries are in testing phase with logger currently deployed in cave.**

##Battery
You can now solder the battery holder to the XIAO board. There are 2 soldering pads on the back of the board that are hard to reach if the board is mounted on the breadboard. A solution can be mounting it upside down to have access to the soldering pads if something goes wrong. Another solution is using soldering a Staking Header on the board so that the parts can be removed for remplacement or troubleshooting. Be careful on picking the right solder pads + and the - for the battery.
The electrical switch goes to the + side of the battery wire.
If your battery holder does not have a on/off switch, then you can insert a switch on the + line, between the battery and the microcontroller.

## Testing the power consumption (Optional)
You can use a multimeter capable of measuring uA, connectimg it between the + of the battery and the + of the board. Another solution is to use a Power Profiler Kit II (Nordic Semiconductor), for a more accurate monitoring.
You should read approximately 10-15ma during operations and approximately 1-1.5mA during the deep sleep mode. If in deep sleep mode you have higher current, check if your wiring is well done. Sometimes cables too long can leak some current.

## Flashing the microcontroller with the CAVEduino software
If it is the first time the DS3231 (RTC Clock) is in use, the clock must be set. To do this use the DS3231 example in the Arduino IDE (preferred way) or temporary uncomment the line:

	Line XX - settare l'orologio

The coin battery on the back of the RTC board will keep the time for years (with some minor drift).
**Then you have to flash the microcontroller again, commenting the line for the clock settings**

Optionally you can modify the name of the logging file for each device, so that it is going to be easier distinguishing the MicroSD cards when retrieving the data from multiple loggers:

	Line XX - Cambiare il nome del file

Finally flash the microcontroller using the file "Name of the File" in the repository.

**Note:** if you want to re-flash the microcontroller after having flashed the CAVEduino software, the Xiao microcontroller  **must be in "Bootloader mode"**. To achieve this you will have to connect the microcontroller to the PC USB while pressing the "Bootloader" button on the Xiao microcontroller.

## Building the case
- Make a 7/8'' hole on the top of the organizer plastic box (better off-centric about 3/4 way toward the hinge).
- Fix the PVC 1/2'' tee screwing tight the PVC 1/2'' male adapter through the hole. The Tee will stay out of the box to host the sensors
- Pass the cables of the BME280 trough the tee, so that the terminations are out of the box on one side of the tee.
- Bend the BME280 header so that this assemblage can easily be inserted in the tee.
- Connect the cables to the BME 280
- Carefully and gently pull the cables from the bottom and place the BME280 sensor in position (in the middle of the tee).
- Stick 4 x zip tie mounters to the upper left corner of the box. You have to place them so that the zip ties will hold the solder breadbord safely.
- Glue the battery holder to the bottom right corner of the box
 
## Operations
Once everything is proper installed in the case and the system is disconnected from the PC, you can operate your new CAVEduino!

- Insert the MicroSD card 
- Insert the batteries

Turn the switch on. You should see the yellow LED on the microcontroller flashing once: this means the CAVEduino is operating properly.

#### Error codes
The integrated LED on the XIAO board is used for detecting errors when switching the CAVEduino on.

**Normal operations:**

- 1 x Led Flashing: CAVEduino is all good! Everything is working fine and you are good to go. This will happen every 30 min.

**Something went wrong:**

The yellow led will repeat the following error codes for 3 times with 3 seconds interval:

- 3 x Led Flashing: RTC not working
- 5 x Led Flashing: MicroSD not mounting
- 10 x Led Flashing: BME280 sensor error 

#### Retrieving data and replacing batteries
The CAVEduino should log data for approximately 6 months  with a set of batteries. Then you have to plan cave trips within this time frame to retrieve the data and swap the batteries. It is suggested to do this operation every 4 months to avoid data gaps  because of a dead battery.

- Before leaving for the trip properly format the MicroSD cards (FAT16 or FAT32 filesystem)
- Be sure that the new batteries are fully charged 
- Make sure both the batteries and the flash cards are well contained and stay dry during caving
- Go caving with your mates and bring with you enough fresh batteries and MicroSD cards for the loggers you have in that cave.
- Open the case
- Turn the power switch OFF
- Replace the MicroSD card
- Replace the batteries
- Turn the power switch ON
- **Be sure to observe that as soon as the CAVEduino is switched on you see the yellow LED flashing ONCE**
- **If the yellow LED flashes 5 times or does not flash at all, then something is not working.** Try to switch off and switch on again. In case the system keeps failing, bring the logger out to fix it.





  
