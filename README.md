# GPS time based Timer
GPS time based Sunrise/Sunset timer

Attiny 104: 1kB FLASH!

- 1004 Bytes Programm
- 16 Bytes timer settings (One Flash Page)
- Free: 6 Bytes :-) (space optimized: AVR-GCC -Os)

##Timereinstellungen via UART Programmieren (COM Port via USB):

###Beispiel Prüfsummenberechnung (vom Payload!)
Byte	CK_A	CK_B

0x03	03		03

0x20	23		26

0x09	2C		52

0x16	42		94

0xFF	41		D5

0x00	41		16

###Paketbeispiel
B9	<-- Paketkennung

03

20	<-- 8:00 Einschalten (Hochdimmen)

09

16	<-- 23:59 Ausschalten (Runterdimmen)

FF	<-- 100% Helligkeit (255 = 100% Duty Cycle)

00	<-- Jede Sekunde aufwährts/abwährts zählen (255 = alle 256 Sek.)

41	<-- Prüfsumme CK_A

16	<-- Prüfsumme CK_B
