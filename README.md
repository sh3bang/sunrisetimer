![Logo](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/ATtiny104.png)
![UBLOX VK16U6](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/UBLOX-vk16u6.jpg)

# GPS based Timer @ ATtiny104 Xplained Nano
Fish tank sunrise and sunset timer

Challenge: 1kB flash!

Result:
- 1004 Bytes programm size
- no eeprom, store timer settings on last flash page (16 Bytes)
- 6 Bytes Free(space optimized compiling with AVR-GCC -Os)

### Dokumente

- [LED Driver](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/ELG-100-C-spec-806035.pdf)
- [u-blox 6 Receiver Description](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/u-blox-6-Receiver-Description.pdf)
- [V.KEL VK16U6 Datasheet](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/VK16u6.rtf)
- [Xplained Nano User Guide](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/Atmel-42671-ATtiny104-Xplained-Nano_User-Guide.pdf)
- [ATtiny104 Datasheet](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/Atmel-42505-8-bit-AVR-Microcontrollers-ATtiny102-ATtiny104_Datasheet.pdf)
- [ATtiny104 PCB](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/ATtiny104_Xplained_Nano_design_documentation_release_rev2.pdf)

##Timereinstellungen via UART Programmieren (COM Port via USB)

Die Auflösung des Timers beträgt 8 Bit, welcher sich pro Sekunde von 0 bis 255 erhöhen kann (Maximale Helligkeit nach ca. 4,6 Min).
Diese Dauer kann bis zu 256 mal verlängert werden (Byte 7 auf 255).

Der IC hat nur ein UART der bereits vom GPS Receiver belegt ist, welcher jedoch nicht permanent die Leitung belegt.
Daher sendet der IC ein "R" für "Ready" wenn er Zeit zum empfangen hat - bzw. wenn er längere Zeit nichts empfangen hat. In diesem Zeitfenster muss das Paket gesendet werden!

###Paketbeispiel
````
B9	<-- Paketkennung
03
20	<-- 8:00 Einschalten (Hochdimmen)
09
16	<-- 23:59 Ausschalten (Runterdimmen)
FF	<-- Max. Helligkeit (255 = 100% Duty Cycle)
00	<-- Dauer des Sonnenauf- und Untergangs
41	<-- Prüfsumme CK_A
16	<-- Prüfsumme CK_B
````

###Beispiel Prüfsummenberechnung
````
Byte	CK_A	CK_B
0x03	03		03
0x20	23		26
0x09	2C		52
0x16	42		94
0xFF	41		D5
0x00	41		16
````