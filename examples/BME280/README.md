## BME 280 WebThings example

In order to be able to use this example, you need to install the WebThings library.

Change the ssid and password to connect to your router

```c++
const char *ssid = "";
const char *password = "";
```

Make sure the BME/BMP 280 has the correct I2C address (0x76)
If it doesn't work or you know you use the other address, change it in

```c++
BME280I2C::Settings settings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_Off,
   BME280::SpiEnable_False,
   BME280I2C::I2CAddr_0x76 // I2C address. I2C specific.
);

```
