# arduino-way-home-meter
A project for displaying the live travel time home considering traffic. Built with an ESP8266

## Part List

- [ESP8266 Wemos D1 Mini*](http://s.click.aliexpress.com/e/8scxud6)
- [4 in 1 Max7219 Dot Matrix display*](http://s.click.aliexpress.com/e/cYd7qa7q)
- [A small servo*](http://s.click.aliexpress.com/e/cDnfleLO)
- [11 Through Hole Neopixels*](http://s.click.aliexpress.com/e/c385tUjI)
- 220pf capacitor
- Passive buzzer
- 1K Resistor
- NPN Transistor
- [D1 Mini Breakout Board](http://blough.ie/d1mb/) (Or use standard protoboard & Screw terminals)

* = Affilate Links


## Wiring

![alt text](https://raw.githubusercontent.com/witnessmenow/arduino-way-home-meter/master/Wiring.PNG)


| D1 Mini  | Dot Matrix Display |
| ------------- | ------------- |
| 5v  | VCC  |
| G  | GND  |
| D7  | DIN  |
| D8  | CS  |
| D5  | CLK  |

| D1 Mini  | Servo |
| ------------- | ------------- |
| 5v  | Red  |
| G  | Brown  |
| D1  | Orange  |

| D1 Mini  | Neopixel |
| ------------- | ------------- |
| 5v  | VCC  |
| G  | GND  |
| D2  | DIN  |

