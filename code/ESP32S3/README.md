# INSTRUCTION FOR ESP32S3 BOARDS
This is a newer and optimized version of the CAVEduino.
This guide is focused on the Adafruit QT Py ESP32S3 board. You will have to check out all the pins. The main difference with other boards (C6, C3...) is the "Sleep" function.
**NOTE** There is a **testing mode** (record data every 1 minute) and a **production mode** (record data every 30 minutes), to change between them you will have to comment and uncomment the lines as per the instruction within the code.

## Equipment
1. Adafruit QT Py ESP32S3
2. BME280 sensor breakboard (with QT connectors) (Temperature, Pressure, Humidity) (I2C)
3. DS3231 RTC (with QT connectors) (Real TIme Clock) (I2C)
4. Adafruit microSD Card BFF (SDI) (can be mounted back to back with the main board)
5. 4 x Alcaline AA batteries (good quality) and holder
6. CR1220 12mm Diameter - 3V Lithium Coin Cell Battery (CR1220)
7. A Class 10 (or better) MicroSD card
7. An electrical switch (you can also find battery holders with an integrated switch)
8. Solder Breadboard and dupont connectors, wires etc.. of every sort
9. A 6'' x 6'' plastic box waterproof (a waterproof small storage parts organizer box will do the job)
10. A PVC tee 1/2'' pipe for plumbing with the the single lateral end threaded
11. A PVC male adapter for electrical conduits
12. 4 x cable zip tie mounting base
13. Sealing Clay or Silicone
14. Silica Gel Desiccant bags

## Wiring
This guide is written for the **Adafruit QT Py ESP32S3** only. Check the [manufacturer Pin-out diagram](https://learn.adafruit.com/adafruit-qt-py-esp32-s3/pinouts) for this board.

The first stage suggested is using a solderless breadboard to familiarize and check the wiring. Then you can solder everything use a solder breadboard after testing goes well.

**Real Time Clock (RTC) - I2C:**
- USE THE STEMMA QT CABLE from the noard to the RTC
- Connect the SQW/INT pin to the A0 pin of the board. **Note!** you need to set the A0 pin as an Input Pullup, and there are 2 ways to do so.
  1) Via software by setting A0 as INPUT PULLUP (already active in the code)
  2) You can do this adding a 10k ohm resistor from the 3V pin to the A0 pin on the board. This solution should provide more stability, but you have to disable the code for the internal pullup: rtc_gpio_pullup_en(WAKEUP_GPIO); rtc_gpio_pulldown_dis(WAKEUP_GPIO);

**BME280 - I2C:**

USE THE STEMMA QT CABLE from the RTC to the BME 280

**SD card - SDI:**

[MOUNT THE MicroSD BOARD back to back with the main board!](https://learn.adafruit.com/adafruit-microsd-card-bff)

**Note**: Just do not connect the PIN "A0" because it will be used for the RTC to wake-up the board from the sleep.

## Microcontroller Flashing and Testing phase
At this stage you want to test the basic assemblage of the hardware. To do so you will be powering the system using the USB port of your computer. Battery pack will be installed in a second stage. **Do not connect the battery and the USB at the same time.**

#### Arduino IDE
Install the [Arduino IDE](https://www.arduino.cc/en/software/?ps_full_site=1)  on your computer.

If you are using Linux you will have to add a file to be able to upload the code to the microcontroller:
[Check this arduino guide](https://support.arduino.cc/hc/en-us/articles/9005041052444-Fix-udev-rules-on-Linux)

if you need to install the Board libraries:
[follow the instruction here to setup the board](https://learn.adafruit.com/adafruit-qt-py-esp32-s3/arduino-ide-setup-99bba7be-288a-490d-b27b-1e63d17882fc)

#### Arduino IDE Libraries
In the Arduino IDE install the following libraries:

- RTClib

- BME 280 Adafruit

#### Sensors Testing Code
Use the code in the "Example Folder" of the Arduino IDE. You will find example code for testing all the parts (RTC, SD, BME280).


####First Troubleshoot
If you have issues while running the code, check every component individually using the example that can be found in the Arduino IDE software. So you can fix one component at the time if you have trouble.

**MicroSD Card issues**: The MicroSD card must be formatted using FAT32 file system. Use a good quality SD card, even if the capacity is oversized (example: SanDisk® High Endurance microSD™ Card 32GB). This avoid a lot of problem of mounting and writing data in a long timeline.

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
If it is the first time the DS3231 (RTC Clock) is in use, the clock must be set. To do this use the DS3231 example in the Arduino IDE (preferred way).

The coin battery on the back of the RTC board will keep the time for years (with some minor drift).
**Then you have to flash the microcontroller again, commenting the line for the clock settings**

You can modify the name of the logging file for each device, so that it is going to be easier distinguishing the MicroSD cards when retrieving the data from multiple loggers:

	#define nomefile "/Logger01.csv"

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

**Note:** It is a good idea to insulate the external from the internal using Sealing Clay or Silicone. Also use the Silica Gel Bags into the box to keep the electronics dry.
 
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

- Before leaving for the trip properly format the MicroSD cards (FAT32 filesystem)
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
