![Logo](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/ATtiny104.png)
![UBLOX VK16U6](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/UBLOX-vk16u6.jpg)

# GPS based Timer @ ATtiny104 Xplained Nano
Zeitschaltuhr für Aquarien mit Sonnenauf- und Untergangssimmulation

### Der ATtiny104 kurz vorgestellt
- 1024 Bytes Flash
- 32 Bytes RAM
- Kein EEPROM!
- 1 UART

### Zur Entwicklerplatine
- Ist nur für absolute Kleinigkeiten zu gebrauchen
- Der ATtiny104 teilt sich ironischer weise mit einem ATmega32U4 das Board und wird über diesen, bspw. mit dem Atmel Studio 7, programmiert
- Dieser "Debugger" ermöglicht es ausserdem via USB Daten an das Programm, an den ATtiny zu senden bzw. zu Empfangen!
- Der Atmel USB Treiber stellt hierfür eine virtuelle, serielle Schnittstelle zur Verfügung

### Zum Programm
- Simmulation von Sonnenaufgang- und Untergang in dem das Licht beim einschalten heller und beim ausschalten dunkler wird
- Zeitpunkt zum ein- und ausschalten sowie die Helligkeitseinstellung und Dauer der Simmulation kann via USB programmiert werden
- Die Einstellungen bleiben auch nach einem Stromausfall erhalten
- Die Uhrzeit wird automatisch via GPS und nicht über DCF77 (Radiofunk) bezogen (Ja, der Emfpang ist auch IN DER WOHNUNG problemlos möglich, sogar sehr Zuverlässig :D)
- Speicherbedingt entdet die Sommer/Winterzeit am letzten Tag des Monats, nicht am letzten Sonntag des Monats!
- Die Einstellungen können "Live" via USB geändert werden, hierbei muss nur darauf geachtet werden, dass das Programm nicht gerade Daten vom GPS Empfänger empfängt (siehe unten)
- Die Helligkeit wird über ein niederfrequentes PWM signal geregelt
- Sollte das GPS Signal verloren gehen, wird das Licht ohne Sonnenuntergang abgeschaltet (Speicherbedingt)
- Zusätzlich kann ein Relais beim ein- und ausschalten des Timers in Betrieb genommen werden


### Sonstige Hinweise
- Kompiliert mit AVR-GCC 4.9.2 vom Atmel Studio 7.0 (build 1188) vom September 2016; Optimierung -Os
- Kompilieren mit anderen Versionen kann sehr warscheinlich zu anderen Programmgrößen führen!
- Die Einstellungen werden auf der letzten "Speicherseite" (16 Bytes) im Flash abgelegt, da kein EEPROM vorhanden ist
- Die maximale Programmgröße darf daher nur 1008 Bytes betragen welche auch akkuat ausgenutzt wird (1008 Bytes)!!!
- Kleiner GPS Nachteil: Zeitzonen sowie Sommer- und Winterzeit gibts bei GPS nicht
- Positionsdaten werden logischerweise auch nicht benötigt, lediglich die Uhrzeit (UTC)
- Speicherbedingt werden keine Interruptroutinen verwendet, daher läuft alles in der Hauptschleife!
- Den Takt gibt der intere Quartz/Oszillator vor welcher bei jedem Model abwechend ausfallen kann und daher sollte ggf. das "Oscillator Calibration Register" angepasst werden (Oszilloskope vorausgesetzt)
- Baudrate ist 9600 (Virtuelle, serielle Schnittstelle als auch vom GPS Modul)
- main.c: Hauptprogramm -> AVR-GCC -> Flash
- checksum.c: Kleines Hilfsprogramm zum erzeugen der Einstellungsdaten die bspw. über das Terminalprogramm HTERM vorgenommen werden, siehe unten
- Mit einem kleinen trick kann der UBlox am PC über die Software "UBlox U-Center" ausgelesen und erforscht werden. Hierfür müssen RX/TX vertauscht werden und der ATtiny104 in den "Reset" -Mode gehen


### Dokumente

- [LED Driver](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/ELG-100-C-spec-806035.pdf)
- [u-blox 6 Receiver Description](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/u-blox-6-Receiver-Description.pdf)
- [V.KEL VK16U6 Datasheet](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/VK16u6.rtf)
- [Xplained Nano User Guide](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/Atmel-42671-ATtiny104-Xplained-Nano_User-Guide.pdf)
- [ATtiny104 Datasheet](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/Atmel-42505-8-bit-AVR-Microcontrollers-ATtiny102-ATtiny104_Datasheet.pdf)
- [ATtiny104 PCB](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/ATtiny104_Xplained_Nano_design_documentation_release_rev2.pdf)

## Timereinstellungen via UART Programmieren (COM Port via USB)

Die Auflösung des Timers beträgt 8 Bit, welcher sich pro Sekunde von 0 bis 255 erhöhen kann (Maximale Helligkeit nach ca. 4,6 Min).
Diese Dauer kann bis zu 256 mal verlängert werden (Byte 7 auf 255).

Der IC hat nur ein UART der bereits vom GPS Receiver belegt ist, welcher jedoch nicht permanent die Leitung belegt.
Daher sendet der IC ein "R" für "Ready" wenn er Zeit zum empfangen hat - bzw. wenn er längere Zeit nichts empfangen hat. In diesem Zeitfenster muss das Paket gesendet werden!

![HTERM](https://raw.githubusercontent.com/sh3bang/sunrisetimer/master/resources/hterm.jpg)


### Paketbeispiel
````
0xB9	<-- Paketkennung
0x03
0x20	<-- 8:00 Einschalten (Hochdimmen)
0x09
0x16	<-- 23:59 Ausschalten (Runterdimmen)
0xFF	<-- Max. Helligkeit (255 = 100% Duty Cycle)
0x00	<-- Dauer des Sonnenauf- und Untergangs
0x41	<-- Prüfsumme CK_A
0x16	<-- Prüfsumme CK_B
````

### Beispiel Prüfsummenberechnung
````
Byte	CK_A	CK_B
0x03	03		03
0x20	23		26
0x09	2C		52
0x16	42		94
0xFF	41		D5
0x00	41		16	<-- Prüfsumme
````