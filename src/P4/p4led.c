#include "p4led.h"

int main(void)
{
	//char zn_utf8[] = "奇幻熊"; //punctuation need input full width
	char en_utf8[] = "Game of Thrones";
	//char en_utf8[] = "Welcome to GH4"; //save in flash

	led_serial_init();
	//p4_led(zn_utf8, strlen(zn_utf8), ZN_FLAG);
	p4_led(en_utf8, strlen(en_utf8), EN_FLAG);

	return 0;
}


