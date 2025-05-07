#include <stdio.h>   
#include <wiringPi.h>  

#define INPUT_PIN 27     // PIR 센서가 연결된 GPIO 핀 번호 정의


int g_nPirState = LOW;   // PIR 센서의 현재 상태를 저장 (초기값: LOW)
int g_nVal = 0;          // PIR 센서로부터 읽은 값을 저장

int main(void) {
    // GPIO 초기화
    if (wiringPiSetupGpio() == -1) { // GPIO를 BCM 모드로 초기화
        return 1;                   // 초기화 실패 시 프로그램 종료
    }

    pinMode(INPUT_PIN, INPUT);       // PIR 센서를 입력 모드로 설정

    // 메인 루프
    while (1) {
        g_nVal = digitalRead(INPUT_PIN); // PIR 센서의 상태 읽기 (HIGH/LOW)

        if (g_nVal == HIGH) {           // 센서가 HIGH 상태일 경우 (움직임 감지됨)
            if (g_nPirState == LOW) {   // 이전 상태가 LOW였다면 (새로운 움직임 감지)
                printf("Motion Detected!!\n"); // 움직임 감지 메시지 출력
            }
            g_nPirState = HIGH;         // 현재 상태를 HIGH로 업데이트
        }
        else {                          // 센서가 LOW 상태일 경우 (움직임 없음)
            if (g_nPirState == HIGH) {  // 이전 상태가 HIGH였다면 (움직임 종료)
                printf("Motion ended.\n"); // 움직임 종료 메시지 출력
            }
            g_nPirState = LOW;          // 현재 상태를 LOW로 업데이트
        }
    }

    return 0; // 프로그램 종료
}