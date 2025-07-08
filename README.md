# little-paw
IoT device built using an ESP32 + AHT20 to track University of Houston's current temperature and humidity! Check out the data real-time at [*URL TBA*]!

<div align="center">
    <img src="./output.gif" height="650px"><br />
</div>

## Hardware Requirements

*   ESP32
*   AHT20
*   Jumper wires
*   Breadboard

<div align="center">
    <img src="./wiring.png" width="400px"><br />
</div>

## Software Requirements

*   ESP-IDF

## Building and Installation

1.  Clone the repository: `git clone https://...`
2.  Open the project in VS Code using ESP-IDF
3.  Set the target: `idf.py set-target esp32`
4.  Set project configuration: `idf.py menuconfig`
5.  Build the project: `idf.py all`
6.  Connect ESP32 and check the serial port: `ls /dev/ttyUSB*`
7.  Flash the serial port: `idf.py -p /dev/ttyUSB0 flash`
8.  Monitor the output: `idf.py -p /dev/ttyUSB0 monitor`

## Acknowledgments

* [DevCon23 - ESP-IDF Getting Started (Beginners Guide to Key Concepts and Resources)](https://www.youtube.com/watch?v=J8zc8mMNKtc&ab_channel=EspressifSystems)
* [ESP32 AHT20 Temperature and Humidity Sensor](https://www.espboards.dev/sensors/aht20/#esp-idf)
* [Adafruit AHT20 Temperature & Humidity Sensor](https://learn.adafruit.com/adafruit-aht20/pinouts)
* [AHT20 Product manuals](https://files.seeedstudio.com/wiki/Grove-AHT20_I2C_Industrial_Grade_Temperature_and_Humidity_Sensor/AHT20-datasheet-2020-4-16.pdf)
* [How I2C Communication Works and How To Use It with Arduino](https://www.youtube.com/watch?v=6IAkYpmA1DQ&ab_channel=HowToMechatronics)
* [Understanding the I2C Bus](https://www.ti.com/lit/an/slva704/slva704.pdf?ts=1751928031170&ref_url=https%253A%252F%252Fwww.google.com%252F)
* [ESP32 with ESP-IDF#5 | ESP32 as I2C Master](https://www.youtube.com/watch?v=Snp6iTu1R7E&ab_channel=ltkdt)
* [unable to flash esp32. the port doesn't exist](https://stackoverflow.com/questions/73923341/unable-to-flash-esp32-the-port-doesnt-exist)