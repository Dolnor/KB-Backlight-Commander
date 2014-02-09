## Keyboard Backlight Commander

This driver is intended to automatically control keyboard backlight on DELL Vostro 3450, 3750, Inspiron 14R N4110, XPS L502x, L702x range of laptops. The driver will enable and disable the keyboard illumination with defined backlight level and defined timeout level at certain time of day, that you specify in ACPI table. By default these laptops have no luminosity sensor, so keyboard backlight is controlled via hotkeys (Fn+F6), which is not really comfortable. For brightness level to settle a HID input of some sort is required (tickle touchpad or keyboard), otherwise the keyboard illumination level will remain at the previous level until you touch the laptop.

### Usage
All the configuration is done via ACPI table (SSDT-9) which you have to edit and compile, then put in your bootloader to inject it into the system. An example SSDT table is included within the code.

- Available backlight levels are: 0x00 - Disabled, 0x01 - Dim, 0x02 - Bright

- Available timeout levels are: 0x00 - Never off, 0x01 - 5sec, 0x03 - 15sec, 0x06 - 30sec, 0x0C - 1min, 0x3C - 5min, 0xB4 - 15min

- Time should be specified in military (24h) format: 0x11 - 17:00, 0x09 - 09:00 etc…

You also need to adjust the default timezone offset inside the plist file of this driver. 
Default is GMT+2 if you leave it at 0 it will mean GMT0 (London) and if you set it to, say, -2 it will be GMT-2. Pretty self explanatory. 

When the time you specified in SSDT hits the keyboard backlight will either be set at desired level or disabled. Upon driver start keyboard backlight timeout will be set according to desired value. This is useful if you have no access to Windows Dell QuickSet utilities to alter the time and you are stuck with 5sec timeout and would like to change it.

### Credits

- ACPI method polling and UNIX timestamp - kozlek from HWSensors project


