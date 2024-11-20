Networking
==========
The gateway includes a couple of network functions to send and receive NMEA data on network connections and to use a WebBrowser for configuration and status display.

To understand the networking functions you always have to consider 2 parts:

The "physical" connection
-------------------------
For the gateway this is always a Wifi connection.
It provides the ability to use it as an access point and/or to connect to another wifi network (wifi client).

Access Point
************
When starting up it will create an own Wifi network (access point or AP) with the name of the system. You connect to this network like to any Wifi Hotspot.
The initial password is esp32nmea2k. You should change this password on the configuration page in the Web UI.
When you connect the gateway will provide you with an IP address (DHCP) - and it will also have an own IP address in this range (default: 192.168.15.1). If this IP address (range) conflicts with other devices (especially if you run multiple gateways and connect them) you can change the range at the system tab on the configuration page.<br>
If you do not need the access point during normal operation you can set "stopApTime" so that it will shut down after this time (in minutes) if no device is connected. This will also reduce the power consumption.

Wifi Client
***********
Beside the own access point the gateway can also connect to another Wifi network. You need to configure the SSID and password at the "wifi client" tab.
On the status page you can see if the gateway is connected and which address it got within the network.

You can use both networks to connect to the gateway. It announces it's services via [bonjour](https://developer.apple.com/bonjour/). So you normally should be able to reach your system using Name.local (name being the system name, default ESP32NMEA2K). Or you can use an app that allows for bonjour browsing.

The "logical" connection
------------------------
After you connected a device to the gateway on either the access point or by using the same Wifi network you can easily access the Web UI with a browser - e.g. using the Name.local url.

To send or receive NMEA data you additionally need to configure some connection between the gateway and your device(s).
The gateway provides the following options for NMEA connections:

TCP Server
**********
When using the TCP server you must configure a connection to the gateway _on the device_ that you would like to connect. The gateway listens at port 10110 (can be changed at the TCP server tab). So on your device you need to configure the address of the gateway and this port. The address depends on the network that you use to connect your device - either the address from the gateway access point (default: 192.168.15.1) - or the address that the gateway was given in the Wifi client network (check this on the status page).<br>
If your device supports this you can also use the Name.local for the address - or let the device browse for a bonjour service.<br>
The TCP server has a limit for the number of connections (can be configured, default: 6). As for any channel you can define what it will write and read on it's connections and apply filters.
If you want to send NMEA2000 data via TCP you can enable "Seasmart out".

TCP Client
**********
With this settings you can configure the gateway to establish a connection to another device that provides data via TCP. You need to know the address and port for that device. If the other device also uses bonjour (like e.g. a second gateway device) you can also use this name instead of the address.
Like for the TCP server you can define what should be send or received with filters.

UDP Reader
**********
UDP is distinct from TCP in that it does not establish a connection directly. Instead in sends/receives messages - without checking if they have been received by someone at all. Therefore it is also not able to ensure that really all messages are reaching their destination. But as the used protocols (NMEA0183, NMEA2000) are prepared for unreliable connections any way (for serial connections you normally have no idea if the data arrives) UDP is still a valid way of connecting devices.<br>
One advantage of UDP is that you can send out messages as broadcast or multicast - thus allowing for multiple receivers.

Small hint for __multicast__:<br>
Normally in the environment the gateway will work you will not use multicast. If you want to send to multiple devices you will use broadcast. The major difference between them are 2 points:<br>
  1. broadcast messages will not pass a real router (but they will be available to all devices connected to one access point)
  2. broadcast messages will always be send to all devices - regardless whether they would like to receive them or not. This can create a bit more network traffic.

Multicast requires that receivers announce their interest in receiving messages (by "joining" a so called multicast group) and this way only interested devices will receive such messages - and it is possible to configure routers in a way that they route multicast messages.

To use the gateway UDP reader you must select from where you would like to receive messages. In any case you need to set up a port (default: 10110). Afterwards you need to decide on the allowed sources:
  * _all_ (default): accept messages from both the access point and the wifi client network - both broadcast messages and directly addressed ones
  * _ap_: only accept messages from devices that are connected to the access point
  * _cli_: only accept messages from devices on the Wifi client network
  * _mp-all_: you need to configure the multicast address(group) you would like to join and will receive multicast from both the access point and the wifi client network
  * _mp-ap_: multicast only from the access point network
  * _mp-cli_: multicast only from the wifi client network

UDP Writer
**********
The UDP writer basically is the counterpart for the UDP reader.
You also have to select where do you want the UDP messages to be sent to.
  * _bc-all_ (default): Send the messages as broadcast to devices connected to the own access point and to devices on the wifi client network
  * _bc-ap_: send the messages as broadcast only to the access point network
  * _bc-cli_: send the messages as broadcast to the wifi client network
  * _normal_: you need to configure a destination address (one device) that you want the messages to be send to
  * _mc-all_: send messages as multicast to both the access point network and the wifi client network. _Hint_: Only devices that configured the same multicast address will receive such messages.
  * _mc-ap_: multicast only to the access point network
  * _mc-cli_: multicast only to the wifi client network.

With the combination of UDP writer and UDP reader you can easily connect multiple gateway devices without a lot of configuration. Just configure one device as UDP writer (with the default settings) and configure other devices as UDP reader (also with default settings) - this way it does not matter how you connect the devices - all devices will receive the data that is sent by the first one.<br>
__Remark:__ be carefull not to create loops when you would like to send data in both directions between 2 devices using UDP. Either use filters - or use TCP connections as they are able to send data in both directions on one connection (without creating a loop).

If you want to forward NMEA2000 data from one gateway device to another, just enable "Seasmart out" at the sender side. This will encapsulate the NMEA2000 data in a NMEA0183 compatible format. The receiver will always automatically detect this format and handle the data correctly.
