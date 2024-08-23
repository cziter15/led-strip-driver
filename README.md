# led-strip-driver
LED strip driver designed to light up my desk
![board](board.png)

## Brief
In the past, without experience on printed circuit boards (PCBs) design, I developed a WiFi-enabled LED strip controller utilizing the ESP8266. Although it functioned, grounding issues caused interference with other devices. 

Currently, I aim to revitalize this project to illuminate my desk. To prevent interference, I have opted for a four-layer PCB, which is quite affordable these days. While a two-layer design is feasible, I see no reason not to utilize a four-layer configuration if the cost remains the same.

# Hardware
The hardware design is relatively straightforward. An LDO is employed to generate a regulated 3.3V supply for the ESP8266 microcontroller, along with several capacitors to ensure stable power delivery to the ESP32. Additionally, one capacitor is utilized to manage the soft-start functionality of the ESP, while two LEDs and a few resistors are included to limit current and control the slope. Although a reduction in emissions (EMC) is anticipated, there is no supplementary ESD protection incorporated. Furthermore, a plug-and-play feature is not intended.

# Software
The application is expected to feature multiple operational modes. It must include functionalities for adjusting color and brightness, in addition to supporting animations managed via MQTT to facilitate integration with home assistant. Another proposed mode is UDP, which aims to enable direct control of individual LEDs from a PC for purposes such as music visualization and other tasks requiring low latency. The core framework for this application will be ksIotFrameworkLib.