#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wiringPi.h>        // WiringPi 라이브러리 헤더
#include <wiringPiSPI.h>     // SPI 통신을 위한 헤더

// 핀 및 SPI 설정
#define CS_MCP3208 8         // MCP3208의 Chip Select(CS) 핀 번호 (GPIO 8)
#define SPI_CHANNEL 0        // SPI 채널 번호 (채널 0 사용)
#define SPI_SPEED 1000000    // SPI 통신 속도 (1MHz)

// MCP3208에서 ADC 값을 읽는 함수
int ReadMcp3208ADC(unsigned char adcChannel)
{
    unsigned char buff[3];   // 전송 및 수신할 데이터 버퍼
    int nAdcValue = 0;       // ADC로부터 읽은 값 저장 변수

    // MCP3208에 전송할 설정 비트 구성
    buff[0] = 0x06 | ((adcChannel & 0x07) >> 2);  // Start 비트와 Single/差動 설정
    buff[1] = ((adcChannel & 0x07) << 6);         // 채널 번호 설정
    buff[2] = 0x00;                                // 데이터 수신을 위한 더미 바이트

    digitalWrite(CS_MCP3208, LOW);                // CS 핀을 LOW로 설정하여 통신 시작
    wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);      // SPI 통신으로 데이터 전송 및 수신
    digitalWrite(CS_MCP3208, HIGH);               // CS 핀을 HIGH로 설정하여 통신 종료

    // 수신된 데이터에서 ADC 값 추출
    buff[1] = 0x0F & buff[1];                     // 상위 4비트만 사용
    nAdcValue = (buff[1] << 8) | buff[2];         // 상위 바이트와 하위 바이트 결합

    return nAdcValue;                             // ADC 값 반환
}

// ADC 값을 거리로 변환하는 함수
int calcDistance(int psdVal) {
    int distance;

    // ADC 값을 이용하여 거리 계산 (센서의 특성에 맞는 수식 사용)
    distance = (67870 / (psdVal - 3)) - 40;       // 거리 계산 공식

    // 측정 가능한 거리 범위 제한 (10cm ~ 80cm)
    if (distance > 80)
        distance = 80;
    else if (distance < 10)
        distance = 10;

    return distance;                              // 계산된 거리 반환
}

int main(void) {
    int psdChannel = 1;   // PSD 센서가 연결된 MCP3208의 채널 번호
    int psdValue = 0;     // ADC로부터 읽은 값 저장 변수
    int distance = 0;     // 계산된 거리 값 저장 변수

    // GPIO 초기화
    if (wiringPiSetupGpio() == -1) {
        fprintf(stdout, "Not start wiringPi: %s\n", strerror(errno));
        return 1;         // 초기화 실패 시 프로그램 종료
    }

    // SPI 초기화
    if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1) {
        fprintf(stdout, "wiringPiSPISetup Failed: %s\n", strerror(errno));
        return 1;         // SPI 초기화 실패 시 프로그램 종료
    }

    pinMode(CS_MCP3208, OUTPUT);  // CS 핀을 출력 모드로 설정

    while (1) {
        psdValue = ReadMcp3208ADC(psdChannel);    // ADC 값 읽기
        distance = calcDistance(psdValue);        // 거리 계산
        printf("Distance: %d (cm)\n", distance);  // 거리 출력
        delay(1000);                              // 1초 대기
    }

    return 0;
}