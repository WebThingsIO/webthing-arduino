## Matrix Display example

You need to install the WebThings Arduino library in order to use this example.

Change the ssid and password to connect to your router

```c++
const char *ssid = "";
const char *password = "";
```

If you want to use more than four matrix displays, you can just change the value of `MAX_DEVICES`

```c++
#define MAX_DEVICES 4
```

If you use another PIN than PIN 5 as CS, you have to change it in the code before

```c++
#define CS_PIN 5
```

The SPI matrix display has to be connected to VSPI of the ESP32.

|  ESP32 Pin  |   | MAX7219 Matrix |
|-------------|---|----------------|
| VCC (3.3V)  | → |    VCC (VDD)   |
|     GND     | → |       GND      |
|  D23 (IO23) | → |   DIN (MOSI)   |
|   D5 (IO5)  | → |     CS (SS)    |
|  D18 (IO18) | → |       CLK      |