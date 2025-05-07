#include <wiringPi.h>

#define LED_RED_1  5
#define LED_RED_2  6


int main (void)
{
	if (wiringPiSetupGpio() == -1)
		return 1;

	pinMode(LED_RED_1,OUTPUT);  	//LED PORT SET
	pinMode(LED_RED_2,OUTPUT);  
	digitalWrite(LED_RED_1,LOW);  	//LED OFF
	digitalWrite(LED_RED_2,LOW);

	while(1)
	{
		digitalWrite(LED_RED_1,HIGH);   //LED1 ON
		digitalWrite(LED_RED_2,LOW);  	//LED2 OFF
		delay(500);
		digitalWrite(LED_RED_1,LOW);  	//LED1 OFF
		digitalWrite(LED_RED_2,HIGH);   //LED2 ON
		delay(500);
	}
	return 0;
}

