# ESP32 LoRa Nunchuck System

## Overview
This project integrates an ESP32 microcontroller with LoRa communication and two Nunchuck connectors, enabling remote control and data transmission applications. The system is designed for long-range communication while leveraging the Wii Nunchuck controllers for user input.

## Features
- **ESP32**: Wi-Fi and Bluetooth-enabled microcontroller
- **LoRa Module**: Long-range communication support
- **Dual Nunchuck Connectors on Separate I²C Buses**
- **Powered via USB**: No LiPo battery required

## Hardware Requirements
- ESP32 development board
- LoRa module (RFM95W-868S2)
- Two Wii Nunchuck controllers
- Nunchuck connectors
- USB power source
- Jumper wires & breadboard (or custom PCB)

## Wiring Diagram (Based on Schematic)
| ESP32 Pin | LoRa Pin | Nunchuck 1 Pin | Nunchuck 2 Pin |
|-----------|---------|---------------|---------------|
| 3.3V      | VCC     | VCC           | VCC           |
| GND       | GND     | GND           | GND           |
| GPIO5     | NSS     | -             | -             |
| GPIO18    | SCK     | -             | -             |
| GPIO19    | MISO    | -             | -             |
| GPIO23    | MOSI    | -             | -             |
| GPIO26    | DIO0    | -             | -             |
| GPIO21    | -       | SDA (I²C 1)   | -             |
| GPIO22    | -       | SCL (I²C 1)   | -             |
| GPIO25    | -       | -             | SDA (I²C 2)   |
| GPIO33    | -       | -             | SCL (I²C 2)   |

## Software Requirements
- Arduino IDE / PlatformIO
- ESP32 board package
- LoRa library (e.g., `RadioHead` or `LoRa.h`)
- Nunchuck library (e.g., `Wire.h`, `ArduinoNunchuk.h`)

## Installation
1. Clone this repository:
   ```sh
   git clone https://github.com/your-repo-name.git
   ```
2. Install dependencies in Arduino IDE:
   - Install ESP32 board support
   - Install required libraries
3. Open the `main.ino` file and upload it to your ESP32 board.

## Usage
1. Connect the hardware components as per the wiring diagram.
2. Power up the ESP32 via USB.
3. Use the Nunchuck controllers to send data via LoRa.
4. Monitor data via serial output.

## Example Code
```cpp
#include <Wire.h>
#include <ArduinoNunchuk.h>
#include <LoRa.h>

TwoWire I2C_1 = TwoWire(0);
TwoWire I2C_2 = TwoWire(1);

ArduinoNunchuk nunchuk1;
ArduinoNunchuk nunchuk2;

void setup() {
    Serial.begin(115200);
    I2C_1.begin(21, 22); // SDA1 = GPIO21, SCL1 = GPIO22
    I2C_2.begin(25, 33); // SDA2 = GPIO25, SCL2 = GPIO33
    
    nunchuk1.init(I2C_1);
    nunchuk2.init(I2C_2);
    
    LoRa.begin(915E6);
}

void loop() {
    nunchuk1.update();
    nunchuk2.update();
    
    LoRa.beginPacket();
    LoRa.print(nunchuk1.analogX);
    LoRa.print(",");
    LoRa.print(nunchuk2.analogX);
    LoRa.endPacket();
    
    delay(100);
}
```

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing
Feel free to submit issues or pull requests for improvements and bug fixes.

## Contact
For inquiries, reach out via GitHub Issues or email `your-email@example.com`. 

---
Happy coding! 🚀
