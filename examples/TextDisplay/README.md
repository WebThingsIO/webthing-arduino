## OLED Display WebThings example

In order to use this example you have to install the WebThings Arduino Library.

Change the ssid and password in order to connect to your router

```c++
const char *ssid = "";
const char *password = "";
```

Connect the SSD1306 OLED display to your ESP32. 
If it has a different I2C address, change the address accordingly.

```c++
 display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
 ```

 If your display has a different resolution, you can change that as well.
 ```c++
 #define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
```

You also need to change the length of the body, headline and subheadline, if your display has a wider size.
This makes it possible to display longer text, if possible.

```c++
#define HEADLINE_MAX_LENGHT 10
#define SUBHEAD_MAX_LENGTH 23
#define BODY_MAX_LENGTH 20
```