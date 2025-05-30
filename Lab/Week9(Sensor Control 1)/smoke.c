#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define CS_MCP3208 8
#define SPI_CHANNEL 0
#define SPI_SPEED 1000000

int ReadMcp3208ADC(unsigned char adcChannel)
{
	unsigned char buff[3];
	int nAdcValue = 0;
	buff[0] = 0x06 | ((adcChannel & 0x07) >> 2);
	buff[1] = ((adcChannel & 0x07) << 6);
	buff[2] = 0x00;
	digitalWrite(CS_MCP3208, 0);
	wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);
	buff[1] = 0x0F & buff[1];
	nAdcValue = (buff[1] << 8) | buff[2];
	digitalWrite(CS_MCP3208, 1);
	return nAdcValue;
}

int main(void)
{
	int smokelChannel = 2;
	int smokeValue = 0;

	if (wiringPiSetupGpio() == -1) {
		fprintf(stdout, "Not start wiringPi: %s\n",
			strerror(errno));
		return 1;
	}
	if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1) {
		fprintf(stdout, "wiringPiSPISetup Failed: %s\n", strerror(errno));
		return 1;
	}
	pinMode(CS_MCP3208, OUTPUT);

	while (1)
	{
		smokeValue = ReadMcp3208ADC(smokelChannel);
		printf("Snoke Sensor Value = % u\n", smokeValue);
			delay(1000);
	}
	return 0;
}