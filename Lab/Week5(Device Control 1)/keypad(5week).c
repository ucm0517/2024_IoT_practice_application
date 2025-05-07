#include <wiringPi.h>  // GPIO 접근을 위한 WiringPi 라이브러리 헤더 파일 포함

// LED와 키패드 핀 정의
#define LED_PIN_1        5      // 첫 번째 LED 핀
#define LED_PIN_2        6      // 두 번째 LED 핀
#define KEYPAD_PB1       23     // 첫 번째 키패드 버튼 핀
#define KEYPAD_PB2       24     // 두 번째 키패드 버튼 핀

#define LED_ON           1      // LED 켜기 명령
#define LED_OFF          0      // LED 끄기 명령
#define MAX_LED_NUM      2      // LED의 개수
#define MAX_KEY_BT_NUM   2      // 키패드 버튼의 개수

// LED 핀을 배열로 저장
const int LedPinTable[2] = {
    LED_PIN_1, LED_PIN_2 
};

// 키패드 버튼 핀을 배열로 저장
const int KeypadTable[2] = {
    KEYPAD_PB1, KEYPAD_PB2 
};

// 키패드 버튼 상태를 읽는 함수
int KeypadRead(void)
{
    int i, nKeypadstate;
    nKeypadstate = 0;
    for(i = 0; i < MAX_KEY_BT_NUM; i++) {
        // 키패드 버튼이 눌렸는지 확인
        if(!digitalRead(KeypadTable[i])) { // 눌린 경우 LOW가 반환됨
            nKeypadstate |= (1 << i); // nKeypadstate의 i번째 비트를 1로 설정
        }
    }
    return nKeypadstate;
}

// LED 제어 함수: LED 번호와 명령(ON/OFF)을 입력받음
void LedControl(int LedNum, int Cmd)
{
    int i;
    for(i = 0; i < MAX_LED_NUM; i++) {
        if(i == LedNum) { // 특정 LED만 제어
            if(Cmd == LED_ON) {
                digitalWrite(LedPinTable[i], HIGH); // LED 켜기
            }
            else {
                digitalWrite(LedPinTable[i], LOW); // LED 끄기
            }
        }
    }
}

int main(void)
{  
    if(wiringPiSetupGpio() == -1) // GPIO 초기화
        return 1; // 초기화 실패 시 프로그램 종료

    int i;
    int nKeypadstate;
    // LED 핀들을 출력 모드로 설정
    for(i = 0; i < MAX_LED_NUM; i++) {
        pinMode(LedPinTable[i], OUTPUT);
    }
    // 키패드 버튼 핀들을 입력 모드로 설정
    for(i = 0; i < MAX_KEY_BT_NUM; i++) {
        pinMode(KeypadTable[i], INPUT);
    }
    while(1) // 무한 루프
    {
        nKeypadstate = KeypadRead(); // 키패드 상태 읽기
        for(i = 0; i < MAX_KEY_BT_NUM; i++) {
            if((nKeypadstate & (1 << i))) { // 키패드가 눌린 경우
                LedControl(i, LED_ON); // 해당 LED 켜기
            }
            else {
                LedControl(i, LED_OFF); // 해당 LED 끄기
            }
        }
    }
    return 0;
}