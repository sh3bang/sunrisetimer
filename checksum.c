#include <stdio.h>

int main(void) {
	
	unsigned int on = 800; // HHMM
	unsigned int off = 2326; // HHMM
	unsigned int dc = 255;
	unsigned int multi = 0;
		
	unsigned char ck_a = 0;
	unsigned char ck_b = 0;

	unsigned char payload[] = {on>>8, on, (off>>8), off, dc, multi};

	printf("%02X", 0xB9);
	
	unsigned int i;
	for(i = 0; i < 6; i++)
	{
		ck_a = ck_a + payload[i];
		ck_b = ck_b + ck_a;
		printf("%02X", payload[i]);
	}
	
	printf("%02X%02X", ck_a, ck_b);
	
	return 0;
}