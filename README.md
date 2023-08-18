# RTL8720DN_mdns
mDNS queries and responses on Realtek AmebaD.
Or to describe it another way: An mDNS Client or Bonjour Client library for the Realtek AmebaD.

This library aims to do the following:
 1. Give access to incoming mDNS packets and decode Question and Answer Records for commonly used record types.
 2. Allow Question and Answer Records for commonly used record types to be sent.

This library is a fork of [mrdunk's esp8266_mdns](https://github.com/mrdunk/esp8266_mdns) with changes applied for RTL8720DN specifics.

Requirements
------------
- An [Realtek AmebaD](https://www.amebaiot.com/en/) WiFi enabled SOC.
- The [AmebaD Arduino](https://github.com/ambiot/ambd_arduino) environment.
- MDNS support in your operating system/client machines:
  - For Mac OSX support is built in through Bonjour already.
  - For Linux, install [Avahi](http://avahi.org/).
  - For Windows, install [Bonjour](http://www.apple.com/support/bonjour/).


Troubleshooting
---------------
Run [Wireshark](https://www.wireshark.org/) on a machine connected to your wireless network to confirm what is actually in flight.
The following filter will return only mDNS packets: ```udp.port == 5353``` .
Any mDNS packets seen by Wireshark should also appear on the ESP8266 Serial console.
