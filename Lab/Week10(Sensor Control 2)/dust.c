#include <stdio.h>       
#include <string.h>      
#include <errno.h>       
#include <wiringPi.h>    
#include <wiringPiSPI.h> 

// 핀 및 SPI 설정
#define CS_MCP3208  8       // MCP3208의 Chip Select(CS) 핀 번호 (GPIO 8)
#define SPI_CHANNEL 0       // SPI 채널 번호 (SPI0 사용)
#define SPI_SPEED   1000000 // SPI 통신 속도 (1MHz)
#define LED_PIN     17      // IR LED 제어를 위한 GPIO 핀 (GPIO 17)

// MCP3208에서 ADC 값을 읽는 함수
int ReadMcp3208ADC(unsigned char adcChannel)
{
    unsigned char buff[3];   // SPI 데이터를 저장할 버퍼 (전송 및 수신)
    int nAdcValue = 0;       // ADC 값을 저장할 변수

    // MCP3208 설정 비트 구성
    buff[0] = 0x06 | ((adcChannel & 0x07) >> 2);  // Start 비트 및 Single/Differential 설정
    buff[1] = ((adcChannel & 0x07) << 6);         // 채널 선택
    buff[2] = 0x00;                               // 더미 데이터

    digitalWrite(CS_MCP3208, 0);                  // CS 핀 LOW로 설정하여 SPI 활성화
    wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);      // SPI 데이터 전송 및 수신
    buff[1] = 0x0F & buff[1];                     // 상위 4비트 추출
    nAdcValue = (buff[1] << 8) | buff[2];         // 상위 바이트와 하위 바이트 결합
    digitalWrite(CS_MCP3208, 1);                  // CS 핀 HIGH로 설정하여 SPI 비활성화

    return nAdcValue;                             // ADC 값 반환
}

// 메인 함수
int main(void)
{
    int dustChannel = 3;      // MCP3208에서 미세먼지 센서가 연결된 채널 번호
    float Vo_Val = 0;         // 센서 출력값 (ADC 값)
    float Voltage = 0;        // 센서 출력 전압 (V)
    float dustDensity = 0;    // 미세먼지 농도 (µg/m³)

    // 센서 동작 타이밍 설정 (단위: µs)
    int samplingTime = 280;   // LED가 켜진 상태 유지 시간 (0.28ms)
    int delayTime = 40;       // 센서 안정화 시간 (0.04ms)
    int offTime = 9680;       // LED가 꺼진 상태 유지 시간 (9.68ms)

    // GPIO 초기화
    if (wiringPiSetupGpio() == -1) {
        fprintf(stdout, "Not start wiringPi: %s\n", strerror(errno));
        return 1;             // 초기화 실패 시 프로그램 종료
    }

    // SPI 초기화
    if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1) {
        fprintf(stdout, "wiringPiSPISetup Failed: %s\n", strerror(errno));
        return 1;             // SPI 초기화 실패 시 프로그램 종료
    }

    pinMode(CS_MCP3208, OUTPUT); // MCP3208의 CS 핀을 출력 모드로 설정
    pinMode(LED_PIN, OUTPUT);    // IR LED 핀을 출력 모드로 설정

    // 메인 루프
    while (1) {
        digitalWrite(LED_PIN, LOW);           // IR LED 켜기
        delayMicroseconds(samplingTime);      // 0.28ms 동안 유지
        Vo_Val = ReadMcp3208ADC(dustChannel); // MCP3208에서 ADC 값 읽기
        delayMicroseconds(delayTime);         // 0.04ms 대기
        digitalWrite(LED_PIN, HIGH);          // IR LED 끄기
        delayMicroseconds(offTime);           // 9.68ms 동안 유지

        // 센서 출력값(ADC)을 전압으로 변환
        Voltage = (Vo_Val * 3.3 / 1024.0) / 3.0; // ADC 값을 전압(V)으로 변환
        // 미세먼지 농도 계산 (공식은 데이터시트에서 제공됨)
        dustDensity = (0.172 * (Voltage) - 0.0999) * 1000; // 단위: µg/m³

        // 결과 출력
        printf("Voltage : %f V\n", Voltage);          // 센서 출력 전압 출력
        printf("Dust Value : %f ug/m3\n", dustDensity); // 미세먼지 농도 출력
        delay(1000);                                 // 1초 대기
    }

    return 0; // 프로그램 종료
}