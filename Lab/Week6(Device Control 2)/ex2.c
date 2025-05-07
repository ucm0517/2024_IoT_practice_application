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
#define BUTTON_STAR_PIN 24  // '*' 버튼 (비밀번호 확인에 사용)
#define BUTTON_HASH_PIN 25  // '#' 버튼 (비밀번호 설정에 사용)

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
int isSettingPassword = 0; // 비밀번호 설정 모드인지 확인

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

// 디바운스 함수: 버튼이 한번만 눌렸는지 확인
int debounce(int pin) {
    if (digitalRead(pin) == LOW) {  // 버튼이 눌린 상태 확인
        delay(50);  // 50ms 대기하여 안정화 확인
        if (digitalRead(pin) == LOW) {  // 여전히 눌려있다면
            while (digitalRead(pin) == LOW);  // 버튼이 눌린 상태가 끝날 때까지 대기
            return 1;  // 유효한 입력
        }
    }
    return 0;  // 유효하지 않은 입력
}

// 디바운스를 적용한 키패드 입력 처리 함수
char readKeypad() {
    // 각 버튼을 디바운스로 확인
    if (debounce(BUTTON1_PIN)) return '1';
    if (debounce(BUTTON2_PIN)) return '2';
    if (debounce(BUTTON3_PIN)) return '3';
    if (debounce(BUTTON4_PIN)) return '4';
    if (debounce(BUTTON5_PIN)) return '5';
    if (debounce(BUTTON6_PIN)) return '6';
    if (debounce(BUTTON7_PIN)) return '7';
    if (debounce(BUTTON8_PIN)) return '8';
    if (debounce(BUTTON9_PIN)) return '9';
    if (debounce(BUTTON0_PIN)) return '0';
    if (debounce(BUTTON_STAR_PIN)) return '*';
    if (debounce(BUTTON_HASH_PIN)) return '#';

    return '\0';  // 아무 버튼도 눌리지 않았을 때 null 반환
}

// 비밀번호 설정 함수
void setPassword() {
    if (inputIndex == 4) {  // 4자리 비밀번호가 입력되었는지 확인
        printf("비밀번호가 설정되었습니다.\n");
        for (int i = 0; i < 4; i++) {
            password[i] = inputPassword[i];  // 입력한 비밀번호를 password 배열에 저장
        }
        isPasswordSet = 1;       // 비밀번호 설정 완료 상태로 변경
        inputIndex = 0;          // 입력 인덱스 초기화
        isSettingPassword = 0;   // 비밀번호 설정 모드 종료
    }
}

// 비밀번호 확인 함수
void checkPassword() {
    if (strncmp(password, inputPassword, 4) == 0) {  // 입력한 비밀번호가 설정된 비밀번호와 일치하는지 확인
        printf("비밀번호가 맞습니다. 문을 엽니다...\n");
        PlayCheerfulSound();  // 경쾌한 소리 재생
        Servo_Open();  // 서보 모터 90도 회전 및 원상태 복귀
        attempts = 0;  // 시도 횟수 초기화
    } else {
        attempts++;
        printf("비밀번호가 틀렸습니다.\n");
        Change_FREQ(1000);  // 경고음 발생
        delay(1000);
        STOP_FREQ();

        if (attempts >= 3) {  // 3회 틀렸을 경우 잠금 기능
            printf("비밀번호 3회 오류. 10초 동안 잠금.\n");
            Change_FREQ(500);  // 경고음
            delay(10000);  // 10초 잠금
            STOP_FREQ();
            attempts = 0;  // 시도 횟수 초기화
        }
    }
    memset(inputPassword, 0, sizeof(inputPassword));  // 입력된 비밀번호 배열을 초기화
    inputIndex = 0;  // 인덱스 초기화
}

// 메인 함수
int main(void) {
    if (wiringPiSetupGpio() == -1) return 1;  // GPIO 초기화 실패 시 종료

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

    printf("비밀번호를 설정하려면 #을 누르세요.\n");

    while (1) {
        char key = readKeypad();  // 키패드 입력 받기

        if (key != '\0') {  // 키가 눌렸을 경우만 처리
            printf("입력된 키: %c\n", key);  //입력된 키 출력
            
            if (key >= '0' && key <= '9') {  // 숫자 키 입력 처리
                if (inputIndex < 4) {
                    inputPassword[inputIndex++] = key;  // 입력된 숫자 저장
                }
            } 
            else if (key == '#') {  // 비밀번호 설정 모드 시작 또는 종료
                if (!isPasswordSet && !isSettingPassword) {  //비밀번호가 아직 설정되지 않았고, 비밀번호 설정 모드가 활성화되지 않은 경우
                    printf("비밀번호 설정 모드입니다. 4자리 숫자를 입력하세요.\n");
                    isSettingPassword = 1;  // 비밀번호 설정 모드 활성화
                } 
                else if (isSettingPassword && inputIndex == 4) {
                    setPassword();  // 비밀번호 설정
                } 
            } 
            else if (key == '*') {  // 비밀번호 확인(이 버튼으로만)
                if (isPasswordSet && inputIndex == 4) {
                    checkPassword();  // 비밀번호 확인
                    inputIndex = 0;   // 입력 인덱스 초기화
                } 
                else {
                    printf("비밀번호를 다 입력하지 않았습니다.\n");
                }
            }
        }

        delay(100);  // 약간의 딜레이 (디바운싱)
    }

    return 0;
}