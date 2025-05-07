#include <stdio.h>
#include <wiringPi.h>
#include <softTone.h>
#include <softPwm.h>
#include <string.h>  // 문자열 관련 함수 사용을 위한 헤더 파일

// 핀 정의
#define BUZZER_PIN 17
#define SERVO_PIN 18
#define BUTTON1_PIN 2  
#define BUTTON2_PIN 3  
#define BUTTON3_PIN 4  
#define BUTTON4_PIN 5  
#define BUTTON5_PIN 6  
#define BUTTON6_PIN 7  
#define BUTTON7_PIN 8  
#define BUTTON8_PIN 20  
#define BUTTON9_PIN 21  
#define BUTTON0_PIN 23  
#define BUTTON_STAR_PIN 24  // '*' 버튼
#define BUTTON_HASH_PIN 25  // '#' 버튼

// 음계 주파수 정의
#define DO_L 523
#define RE 587
#define MI 659
#define FA 698
#define SOL 784
#define RA 880
#define SI 987
#define DO_H 1046

// 비밀번호 변수
char password[4];  // 설정된 비밀번호 저장
char inputPassword[4];  // 입력된 비밀번호 저장
int inputIndex = 0;  // 입력된 자리
int isPasswordSet = 0;  // 비밀번호가 설정되었는지 여부
int attempts = 0;  // 시도 횟수

// 디바운싱에 사용할 이전 상태 변수
int lastButtonState = HIGH;

// 부저 및 서보 모터 제어 함수
void Change_FREQ(unsigned int freq) {
    softToneWrite(BUZZER_PIN, freq);  // 부저 주파수 변경
}

void STOP_FREQ(void) {
    softToneWrite(BUZZER_PIN, 0);  // 부저 멈춤
}

void Buzzer_Init(void) {
    softToneCreate(BUZZER_PIN);  // 부저 초기화
    STOP_FREQ();
}

void Servo_Init(void) {
    softPwmCreate(SERVO_PIN, 0, 200);  // 서보 모터 초기화
}

void Servo_Open(void) {
    softPwmWrite(SERVO_PIN, 25);  // 서보 모터 90도 회전
    delay(2000);  // 2초 대기
    softPwmWrite(SERVO_PIN, 15);   // 서보 모터 원래 상태로 복귀
}

// 경쾌한 소리 재생 함수
void PlayCheerfulSound(void) {
    // 경쾌한 소리 주파수 배열
    int melody[] = {DO_H, SOL, MI, SOL, DO_H};
    int length = sizeof(melody) / sizeof(melody[0]);

    for (int i = 0; i < length; i++) {
        Change_FREQ(melody[i]);  // 각각의 음 재생
        delay(200);  // 각 음을 200ms 동안 재생
    }

    STOP_FREQ();  // 소리 멈춤
}

// 키패드 입력 처리 함수 (디바운싱 적용)
char readKeypad() {
    int buttonState = HIGH;

    // 각 버튼에 해당하는 GPIO 핀 상태를 읽고, 눌린 버튼의 값을 반환
    if (digitalRead(BUTTON1_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON2_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON3_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON4_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON5_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON6_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON7_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON8_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON9_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON0_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON_STAR_PIN) == LOW) buttonState = LOW;
    if (digitalRead(BUTTON_HASH_PIN) == LOW) buttonState = LOW;

    // 버튼이 눌리고 나서 떼어졌을 때만 값을 반환 (디바운싱 처리)
    if (buttonState == LOW && lastButtonState == HIGH) {
        delay(200);  // 디바운싱을 위한 지연
        lastButtonState = LOW;

        // 눌린 버튼에 따른 값 반환
        if (digitalRead(BUTTON1_PIN) == LOW) return '1';
        if (digitalRead(BUTTON2_PIN) == LOW) return '2';
        if (digitalRead(BUTTON3_PIN) == LOW) return '3';
        if (digitalRead(BUTTON4_PIN) == LOW) return '4';
        if (digitalRead(BUTTON5_PIN) == LOW) return '5';
        if (digitalRead(BUTTON6_PIN) == LOW) return '6';
        if (digitalRead(BUTTON7_PIN) == LOW) return '7';
        if (digitalRead(BUTTON8_PIN) == LOW) return '8';
        if (digitalRead(BUTTON9_PIN) == LOW) return '9';
        if (digitalRead(BUTTON0_PIN) == LOW) return '0';
        if (digitalRead(BUTTON_STAR_PIN) == LOW) return '*';
        if (digitalRead(BUTTON_HASH_PIN) == LOW) return '#';
    }
    
    // 버튼이 떼어진 상태일 때
    if (buttonState == HIGH && lastButtonState == LOW) {
        lastButtonState = HIGH;  // 다시 버튼을 눌렀을 때만 작동하게 함
    }

    return '\0';  // 아무 버튼도 눌리지 않았을 때 null 반환
}

int main(void) {
    if (wiringPiSetupGpio() == -1) return 1;

    // 각 키패드 버튼 핀을 입력 모드로 설정
    pinMode(BUTTON1_PIN, INPUT);
    pinMode(BUTTON2_PIN, INPUT);
    pinMode(BUTTON3_PIN, INPUT);
    pinMode(BUTTON4_PIN, INPUT);
    pinMode(BUTTON5_PIN, INPUT);
    pinMode(BUTTON6_PIN, INPUT);
    pinMode(BUTTON7_PIN, INPUT);
    pinMode(BUTTON8_PIN, INPUT);
    pinMode(BUTTON9_PIN, INPUT);
    pinMode(BUTTON0_PIN, INPUT);
    pinMode(BUTTON_STAR_PIN, INPUT);
    pinMode(BUTTON_HASH_PIN, INPUT);

    // 내부 풀업 저항 활성화
    pullUpDnControl(BUTTON1_PIN, PUD_UP);
    pullUpDnControl(BUTTON2_PIN, PUD_UP);
    pullUpDnControl(BUTTON3_PIN, PUD_UP);
    pullUpDnControl(BUTTON4_PIN, PUD_UP);
    pullUpDnControl(BUTTON5_PIN, PUD_UP);
    pullUpDnControl(BUTTON6_PIN, PUD_UP);
    pullUpDnControl(BUTTON7_PIN, PUD_UP);
    pullUpDnControl(BUTTON8_PIN, PUD_UP);
    pullUpDnControl(BUTTON9_PIN, PUD_UP);
    pullUpDnControl(BUTTON0_PIN, PUD_UP);
    pullUpDnControl(BUTTON_STAR_PIN, PUD_UP);
    pullUpDnControl(BUTTON_HASH_PIN, PUD_UP);

    Buzzer_Init();  // 부저 초기화
    Servo_Init();   // 서보 모터 초기화

    printf("비밀번호를 설정하거나 입력하세요.\n");

    while (1) {
        char key = readKeypad();  // 키패드 입력 받기

        if (key != '\0') {  // 키가 눌렸을 경우만 처리
            printf("입력된 키: %c\n", key);
        }

        delay(100);  // 약간의 딜레이 (디바운싱)
    }

    return 0;
}
