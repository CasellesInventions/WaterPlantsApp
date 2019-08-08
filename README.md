# Welcome to WaterPlantsApp!

You are free to use my code and forget about watering the plants or just don't be worried when you leave for holidays.

The App is based on an **Arduino** board that can be controlled by an **Android** device thru the network. I have been using it for two years and I have been able to water my plants when flying in the middle of the Atlantic Ocean at 1000 km/h and 10 km above the sea level! There was WiFi on the plane... :) 

You can find more info in my site:  [https://sites.google.com/view/caselles-inventions/applications/water-plants-system](https://sites.google.com/view/caselles-inventions/applications/water-plants-system)


# Guidelines

Basically you need to adapt the code for your own purpose with some parameters. If you just want to run it automatically instead of using and Android App, you can modify the code to run always in AUTO mode and water the plants when the moisture sensor detect low levels...(or just make your own version watering the plants in regular intervals!)

To make it work you need to focus on different matters: Arduino app, Android app and maybe some networking configuration. But don't worry, it is easy! 

## Arduino

You will need an Arduino board (it could be Arduino UNO, but the code is not very memory efficient written, so MEGA could be better...).
To use all the tools implemented you will need this sensors/components:

 - **Ethernet board** : To connect to the network (next version could be WiFi)
 - **PING sensor** : It is used to measure the water level in the tank
 - **Relay** : To turn on/off the water pump
 - **Moisture sensors** : In my version I am using 5 sensors for 5 different plants
- **Power Supply**: 9V is the ideal, but for example I have been using a 12V transformer to give more power to the pump I have. The Arduino barrel input can take up to 12V (it is the limit, so under your own risk).
- **Water Pump**: This version has one water pump (like the aquarium ones). I have the pump inside the bucket and it pumps the water thru one plastic pipe/hose. I made small holes on the hose and I close the output. Just make more holes where you have the plants that need more water.

NOT COMPULSORY (but used):
 - **Transistor**: Not compulsory, but I use it to open/close the moisture sensors circuit to avoid corrosion. If there is current all the time thru the sensors, in the long run with the water of the plant, it will get rusty. I am using a FET N in this prototype. **220 Ohm** resistor needed.
- Hardware Reset: I send a signal thru a cable directly to the HW reset input. What if something goes wrong?? so..just reset the Arduino could help... For this, you need a **1000 Ohm** resistor 

### Implementation:
The Arduino is reading the information from the sensors and actuates to the pump to water the plants, but also is a web server where the Android application can connect. The functions contained in the code enable us to:

- Water the plants for some seconds. Parameter to set the seconds:
	- ``#define WATERING_SECONDS 5`` In seconds. This is also used when watering in AUTO mode.
- Measure the water level of the tank. To calibrate the distances, tune:
	- ``#define MAX_WATER_DISTANCE_CM 100`` In cm, the distance from the PING sensor to the bottom of the bucket (min water level = max distance to the water from the sensor).
	- ``#define MIN_WATER_DISTANCE_CM 73`` In cm, the distance from the PING sensor to the maximum level of water.
- Moisture levels:
	- ``#define N_MOISTURE_SENSORS 5`` Number of moisture sensors.
	- ``#define TRANSISTOR_SECONDS 2`` Number of seconds to close the moisture sensors circuit. It is used to warm up the current flux thru moisture sensors (avoid corrosion). Do not need to change (just if the time for the reading increases).
	- ``#define MAX_R_MOISTURE 1023``, ``#define MIN_R_MOISTURE 250`` This parameters tune the min and max resistance of the moisture sensors to scale the values between 0 and 100%. It could differ depending of the model.
	- ``#define AVG_READINGS 5`` To have better readings, it is a good technique to read several times the sensor measurements and average the values. This parameter tells how many readings you want to do.

- Water the plants when the average moisture level of the plants is below certain level. This levels are checked regularly when it is in AUTO mode ON. The parameters to tune are:
	- ``#define AUTO_CHECK_TIMER 60`` In minutes: interval for checking the moisture.
	- ``#define AUTO_AVG_TH_PERCENTAGE 40`` Percentage of the average below it waters the plants.
	- ``#define COEFF_AVG_SENSOR_1 1``, ``#define COEFF_AVG_SENSOR_2 1`` ... These coefficients can be use to tune how to do the average of the moisture of the plants. For example, if the coefficient for the first plant is 0.5 and the rest is 1, the moisture level of the first plant is weighted half in the average calculation. This can be adjusted depending of how much water needs every plant.

## Android App (MIT App Inventor)

The Android App is created with the online tool MIT App Inventor. ( Create a free account and just import the .aia file from the repository).

THE ANDROID APP IS NOT NEEDED SINCE THE ARDUINO IS ACTING AS A WEB SERVER, SO YOU CAN ACCESS IT DIRECTLY FROM AN INTERNET BROWSER AND YOU WILL USE IT AS A WEB PAGE. THE ANDROID APP IS JUST A USER FRIENDLY LAYER, BUT OF COURSE IT IS MUCH NICER TO USE THE APP  ;)

You must edit the file with your own **public static IP** (or DDNS service, explained bellow) and **PORT**. In the MIT app inventor go to blocks and change "your_IP_here" and "your_PORT_here" with your parameters. In case of a DDNS, just write your DDNS address instead of the IP.

In case you want to use the network control only local, write your local IP and PORT. Then you will be only able to access the server from your home network.

At MIT App Inventor tool you can check how to install the app in your Android device. You can use a simulator, AI companion or just build it and save it in your computer as .apk installer. Then move this file to your device and install it (give it permission for unknown sources). There is also a function to get it thru a QR code.


## Networking
First of all you need to configure the communication between the Arduino and your router. You need to connect thru an ethernet cable your router to the Arduino shield.
In the Arduino code you must tell which **local IP** and **PORT** you want to use for your web server:

``//Internal MAC/IP configuration:``
``byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // GENERAL MAC``
``IPAddress ip(192, 168, 0, 000); // WRITE HERE YOUR LOCAL IP FOR THE SERVER``

``//Port HTTP:``
``EthernetServer server(81);``

You do not need to change the mac. For the IP just choose one local IP address that is not being used, you can check that in you router configuration. To enter in the configuration you can ask your internet provider but it usually you can try to access http://192.168.0.1 or similar. This link gives some information: [https://www.lifewire.com/accessing-your-router-at-home-818205](https://www.lifewire.com/accessing-your-router-at-home-818205).
HTTP port is usually 80, but you can choose another one if you prefer too. 

With the parameters set in your Arduino you need to forward the ports in your router. You can find information online about how to do it and it could vary depending of your router model. Basically, access the router configuration as explained before and go to Port Forwarding section, then select local IP and PORT to let you connect to your web server. This can be from an external WiFi in another country, or just from your mobile internet provider. 

If you do not have a static public IP, I recommend to you to use a DDNS service, there are several online providers for free. This means that even if you ISP (internet provider) change you public IP, you can refer and update you public IP to a web address. Some routers even send a message to this services updating your new public IP.

Enjoy!







