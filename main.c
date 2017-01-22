
// Systemtakt in Hz - Definition als unsigned long beachten
// Ohne ergeben sich unten Fehler in der Berechnung
#define F_CPU 2000000UL // 2 MHz clock speed

// Baudrate
#define BAUD 9600UL

// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.

#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
#error "Systematischer Fehler der Baudrate groesser 1% und damit zu hoch!"
#endif

#define VALID_UTC 2

#define END_TIMER 107
#define END_TIMEUTC 28

#include <avr/io.h>
#include <avr/pgmspace.h>

static void PAGE_Erase(uint16_t address)
{
	asm volatile (
	"ldi  r19,0x18  \n\t"
	"out  0x33,r19  \n\t"   /* set NVMCMD to PAGE_ERASE; */
	"ldi  r19,0xE7  \n\t"
	"out  0x3C,r19  \n\t"   /* CPP = 0xE7 NVM self-programming enable */
	/* trigger page erase within four clock cycles.*/
	"ldi  r22,0x00  \n\t"
	"st  Z+,r22     \n\t"
	"nop            \n\t"
	"nop  "
	:                       /* No outputs. */
	:"z" (address)
	:"r19", "r22"
	);
	while (NVMCSR & (1 << NVMBSY) );  // Wait while memory is being programmed
}

static void NVM_Write(uint16_t address, uint16_t word)
{
	uint8_t hbyte = word>>8;
	asm volatile (
	"ldi r19,0x1D    \n\t"
	"out 0x33,r19    \n\t"  /* set NVMCMD to WORD_WRITE; */
	"ldi r19,0xE7    \n\t"
	"out 0x3C,r19    \n\t"  /* CPP = 0xE7 NVM self-programming enable */
	/* trigger value write within four clock cycles.*/
	"st Z+,%1        \n\t"
	"st Z,%2         \n\t"
	"nop             \n\t"
	"nop "
	:                       /* No outputs. */
	:"e" (address),"r"(word),"r"(hbyte)
	:"r19"
	);
	while (NVMCSR & (1 << NVMBSY) );  // Wait while NVM memory is busy, being programmed.
}

struct checksum {
	uint8_t a,
	b;
};

struct payload_timer {
	uint8_t on_high,
	on_low,
	off_high,
	off_low,
	dc_max,
	dc_skipping;
};

struct payload_ubx_nav_timeutc {
	uint8_t month,
	day,
	hour,
	minute,
	flags;	
};

struct package {
	struct payload_timer timer;
	struct payload_ubx_nav_timeutc timeutc;
	struct checksum cks;
};


static void uart_putc(unsigned char c)
{
	while (!(UCSRA & (1<<UDRE)));  /* warten bis Senden möglich */
	UDR = c;                       /* sende Zeichen */
}

static void enable_nav_timeutc()
{	
	// header
	uart_putc(0xB5); // Sync Char 1
	uart_putc(0x62); // Sync Char 2
	uart_putc(0x06); // UBX Class CFG
	uart_putc(0x01); // UBX-CFG-MSG
	
	// Payload Length (little endian!)
	uart_putc(0x03);
	uart_putc(0x00);
	
	// Payload 
	uart_putc(0x01); // UBX Class NAV
	uart_putc(0x21); // UBX-NAV-TIMEUTC
	uart_putc(0x01); // rate
	
	// Checksum
	uart_putc(0x2D);
	uart_putc(0x85);

	/*
	calculation:
	Byte	CK_A	CK_B
	0x06	06		06
	0x01	07		0D
	0x03	0A		17
	0x00	0A		21
	0x01	0B		2C
	0x21	2C		58
	0x01	2D		85	<--- Checksum!
	*/	
}

int main(void)
{
	DDRA = (1<<DDRA0)|(1<<DDRA1)|(1<<DDRA5);
	PORTA = (1<<PORTA5);
	
	/************************************************************************/
	/* CPU-Takt einstellen                                                  */
	/************************************************************************/
	
	/*Oscillator Calibration Register*/
	OSCCAL = 5;
	
	/*Configuration Change Protection Register*/
	CCP = 0xD8;
	
	/*Clock Prescaler Register*/
	CLKPSR = CLKPS1;

	/************************************************************************/
	/* PWM Konfigurieren                                                    */
	/************************************************************************/
	
	TCCR0A = (1<<COM0B1)|(1<<COM0B0)|(1<<WGM00); // OC0B aktivieren, 8-bit PWM, output high level
	TCCR0B = (1<<CS01)|(1<<WGM02);// PWM Frequenz = 278Hz @ 2MHz clock / 8 (Timer Prescaler)
	
	/************************************************************************/
	/* UART Konfigurieren                                                   */
	/************************************************************************/
	
	/*USART Baud Rate Register Low and High byte*/
	//UBRRH = UBRR_VAL >> 8;
	UBRRL = UBRR_VAL & 0xFF;
	
	/*USART Control and Status Register B*/
	UCSRB = (1<<RXEN)|(1<<TXEN); // RX und TX einschalten
	
	/*USART Control and Status Register C*/
	UCSRC = (1<<UCSZ1)|(1<<UCSZ0); // Asynchron 8N1
	
	/************************************************************************/
	/* UBX-NAV-TIMEUTC Aktivieren wenn Ublox bereit                         */
	/************************************************************************/
	while(!(PINA & (1<<PINA6)));
	enable_nav_timeutc();
	
	
	unsigned char chr;		

	struct package capture;

	struct checksum cks;
	cks.a = 0;
	cks.b = 0;

	uint16_t date_lsb;
	uint16_t time;
	uint16_t timer_on;
	uint16_t timer_off;
	uint16_t settings = 0;
	
	uint8_t duty_cyle_max = 0;
	uint8_t skipping = 0;
	uint8_t skipped = 0;
	uint8_t offset = 0;
	uint8_t idle = 0;

	while(1)
	{
		/************************************************************************/
		/* Auf Empfang warten                                                   */
		/************************************************************************/

		if(!(UCSRA & (1<<RXC)))
		{
			// Wenn nach einiger Zeit nichts Empfangen, dann Leitung frei -> "R" Senden
			if(idle >= 250){
				if(idle == 250)
				{
					while (!(UCSRA & (1<<UDRE)));  /* warten bis Senden möglich */
					UDR = 'R';                     /* sende Zeichen "R" für "Ready" */
					PORTA |= (1<<PORTA5); // Board LED aus
					idle++;
				}
			} else {
				idle++;	
			}
		}
		else
		{
			idle = 0;
			
			chr = UDR;
			if(
				(offset == 1 && chr != 0xB5) || //Sync Char 1
				(offset == 2 && chr != 0x62) || //Sync Char 2
				(offset == 3 && chr != 0x01) || //UBX Class NAV
				(offset == 4 && chr != 0x21)	//UBX-NAV-TIMEUTC
			){
				if(offset == 1 && chr == 0xB9) {  // Sync Char
					offset = 99;
				} else {
					offset = 0;
				}
			}
			else
			{
				switch(offset)
				{										//	Package			Function					Payload Offset
					case 100:							//	timer			Timer On (High Byte)		1
						capture.timer.on_high = chr;
						break;

					case 101:							//	timer			Timer On (Low Byte)			2
						capture.timer.on_low = chr;
						break;

					case 102:							//	timer			Timer Off (High Byte)		3
						capture.timer.off_high = chr;
						break;

					case 103:							//	timer			Timer Off (Low Byte)		4
						capture.timer.off_low = chr;
						break;
						
					case 104:							//	timer			Max. Duty Cycle				5
						capture.timer.dc_max = chr;
						break;
						
					case 105:							//	timer			skip increase every x Sec.	6
						capture.timer.dc_skipping = chr;
						break;
											
					case 21:							//	nav_timeutc		UTC Month					14
						capture.timeutc.month = chr;
						break;
				
					case 22:							//	nav_timeutc		UTC Day						15
						capture.timeutc.day = chr;
						break;

					case 23:							//	nav_timeutc		UTC Hour					16
						capture.timeutc.hour = chr;
						break;
					
					case 24:							//	nav_timeutc		UTC Hour					17
						capture.timeutc.minute = chr;
						break;
					
					case 26:							//	nav_timeutc		Valid Flags					19
						capture.timeutc.flags = chr;
						break;
					
					case (END_TIMER-1):					//	timer			CK_A
					case (END_TIMEUTC-1):				//	nav_timeutc		CK_A
						capture.cks.a = chr;
						break;
					
					case END_TIMER:						//	timer			CK_B
					case END_TIMEUTC:					//	nav_timeutc		CK_B
						capture.cks.b = chr;
						break;				
				}
			}

			if((offset >= 3 && offset <= (END_TIMEUTC-2)) || (offset >= 100 && offset <= (END_TIMER-2))){
				cks.a += chr;
				cks.b += cks.a;
			}

			if(offset == END_TIMEUTC || offset == END_TIMER)
			{
				if(cks.a == capture.cks.a && cks.b == capture.cks.b) // Checksum check
				{
					if(offset == END_TIMER)
					{
						PAGE_Erase(0x43FB);
						NVM_Write(0x43FA, ((capture.timer.dc_skipping << 8) | capture.timer.dc_max));
						NVM_Write(0x43FC, ((capture.timer.on_high << 8) | capture.timer.on_low));
						NVM_Write(0x43FE, ((capture.timer.off_high << 8) | capture.timer.off_low));

						while (!(UCSRA & (1<<UDRE)));  /* warten bis Senden möglich */
						UDR = 'U';                     /* sende Zeichen "U" für "Updated" */
					}
					else if(offset == END_TIMEUTC && (capture.timeutc.flags & (1<<VALID_UTC))) // UTC time check
					{
						settings = pgm_read_word(1018);						
						skipping = (settings >> 8);
						
						if(skipped < skipping)
						{
							skipped++;
						}
						else
						{
							skipped = 0;
							
							/************************************************************************/
							/* Winterzeit von Ende Oktober bis Ende März (eigentl. letzter Sonntag) */
							/************************************************************************/
						
							date_lsb = capture.timeutc.month*100+capture.timeutc.day;

							if(date_lsb >= 1031 || date_lsb < 331){ // statisch von 31.10 bis 31.03
								time = 200; // UTC + 2 Std.
							}
							else {
								time = 100; // UTC + 1 Std.
							}

							time += capture.timeutc.hour*100+capture.timeutc.minute;
							
							/************************************************************************/
							/* Timereinstellungen aus FLASH laden                                   */
							/************************************************************************/
						
							timer_on = pgm_read_word(1020);
							timer_off = pgm_read_word(1022);
						
							/************************************************************************/
							/*	time		time_on		time_off	bool	                        */
							/*	15:00		21:00		18:00		true							*/
							/*	18:00		15:00		21:00		true							*/
							/*	21:00		18:00		15:00		true							*/
							/*	15:00		18:00		21:00		false							*/
							/*	18:00		21:00		15:00		false							*/
							/*	21:00		15:00		18:00		false							*/
							/************************************************************************/

							if((time >= timer_on && time < timer_off) || ((time >= timer_on || time < timer_off) && timer_on > timer_off)) {
								duty_cyle_max = settings;
								if(OCR0BL < duty_cyle_max) {
									OCR0BL += 1;
								}
								else if(OCR0BL > duty_cyle_max) {
									OCR0BL -= 1;
								}
								PORTA |= (1<<PORTA0); // CO2 und Lüfter an
							}
							else if(OCR0BL > 0) {
								OCR0BL -= 1;
							}
							else {
								PORTA &= ~(1<<PORTA0); // CO2 und Lüfter aus
							}
						}
						PORTA &= ~(1<<PORTA5); // Board LED an (gültige uhrzeit)
					}
				}

				cks.a = 0;
				cks.b = 0;
				offset = 0;
			}
			
			offset++;
		}
	}
}