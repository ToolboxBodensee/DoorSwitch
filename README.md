# DoorSwitch

**Entwicklung nur auf [GitLab](https://gitlab.com/ToolboxBodensee/microcontroller/DoorSwitch). Auf GitHub befindet sich lediglich ein Mirror**

Firmware for an ESP8266 connected to a switch at the door, updating the space status

Additional Infos are here: [toolbox-bodensee.de/en/projekte/tuererkennung/](https://toolbox-bodensee.de/en/projekte/tuererkennung/)

## compile with platform io

```pio run```

## compile with platform io

```
pio run -t upload
//or as OTA
pio run -t upload --upload-port 172.23.16.247```
