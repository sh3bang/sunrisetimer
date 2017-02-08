![Logo](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/ATtiny104.png)

# GPS based Timer @ ATtiny104 Xplained Nano
Sunrise and sunset timer for fish tanks

challenge: 1kB flash!

Result:
- 1004 Bytes programm size
- no eeprom, store timer settings on last flash page (16 Bytes)
- Free: 6 Bytes (space optimized compiling with AVR-GCC -Os)

##Timereinstellungen via UART Programmieren (COM Port via USB)

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

Die Auflösung des Timers beträgt 8 Bit, welcher sich pro Sekunde von 0 bis 255 erhöhen kann (Maximale Helligkeit nach ca. 4,6 Min).
Diese Dauer kann bis zu 256 mal verlängert werden (Byte 7 auf 255).

PS: der IC hat nur ein UART der bereits vom GPS Receiver belegt ist, welcher jedoch nicht permanent die Leitung belegt.
Daher sender der IC ein "R" für "Ready" wenn er Zeit zum empfangen hat - bzw. wenn er längere Zeit nichts empfangen hat. In diesem Zeitfenster muss das Paket gesendet werden!