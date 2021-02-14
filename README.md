# RECU    
### Renix ECU Reader

The **RECU** Android app is developed with [B4A](https://www.b4x.com/b4a.html) by Anywhere Software.  The ESP32 microcontroller attached to the **RECU** control board is programmed with [Arduino](https://www.arduino.cc). The app connects via bluetooth to the ESP32 that is connected to the Jeep diagnostic port via serial link.

## RECU control board
The circuit to gather information from the ECU is a modified version of a design by John Eberle of TractorEnvy.com.  Board was designed using freeware version of Diptrace and made by [OshPark](https://oshpark.com/shared_projects/BllhAg80).  The ECU packet structure is based on the work of Nick Risley of NickInTimeDesign.com.

<img src="readme/Board.png" alt="Board.png" width="150px" height="150px"> <img src="readme/Board_Assembly.png" alt="Board_Assembly.png" width="200px" height="150px">
<img src="readme/Board_Assembly2.png" alt="Board_Assembly2.png" width="200px" height="150px"> <img src="readme/Board_Connector.png" alt="Board_Connector.png" width="150px" height="150px"> <img src="readme/Board_Connector2.png" alt="Board_Connector2.png" width="200px" height="150px">

### Bill of Materials
*1: RECU printed circuit boardd
*1: 03-09-2151 Molex 15 Circuit Plug
*4: 02-09-2134 Molex PC Tail Male
*1: RenCom R-78E6.5-0.5 DC/DC Converter 0.5A 6.5V
*1: Bel 0ZRR0030FF1E PPTC Resettable Fuse 300mA
*1: Four position PCB mount screw terminals
*1: ESP-32S Dual Development Board
*1: 1N5817 Schottky Rectifier Diode
*1: 150 Ohm Resistor
*1: 2200 Ohm Resistor
*1: 4700 Ohm Resistor
*1: 3300 Ohm .25w Resistor
*1: 1000 Ohm .25w Resistor
*1: 2N3904 Transistor
*1: Four 18AWG conductor cable, 6-8 ft
*1: Printed Connector cover (optional)

## Screenshot of RECU running on Android phone
Find [RECU](https://play.google.com/store/apps/details?id=cmarsoft.recu) on Google Play.

<img src="readme/Screenshot_RECU_1.png" alt="RECU_1.png" width="200px" height="400px">
