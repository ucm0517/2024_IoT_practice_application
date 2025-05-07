#include <stdio.h>
#include <wiringPi.h>
#include <softTone.h>
#include <softPwm.h>
#include <string.h>
#include <lcd.h>  // LCD 제어를 위한 헤더파일

// 핀 정의
#define BUZZER_PIN 17         // 부저 핀
#define SERVO_PIN 18          // 서보 모터 핀

#define BUTTON1_PIN 27        // 키패드 버튼 핀 1
#define BUTTON2_PIN 22        // 키패드 버튼 핀 2

#define BUTTON3_PIN 10        // 키패드 버튼 핀 3
#define BUTTON4_PIN 9         // 키패드 버튼 핀 4
#define BUTTON5_PIN 11        // 키패드 버튼 핀 5

#define BUTTON6_PIN 5         // 키패드 버튼 핀 6
#define BUTTON7_PIN 6         // 키패드 버튼 핀 7
#define BUTTON8_PIN 13        // 키패드 버튼 핀 8
#define BUTTON9_PIN 19        // 키패드 버튼 핀 9
#define BUTTON0_PIN 26        // 키패드 버튼 핀 0

#define BUTTON_DOWN_PIN 23    // 메뉴 스크롤(아래) 버튼 핀
#define BUTTON_ENTER_PIN 24   // 메뉴 선택(엔터) 버튼 핀

// 음계 주파수 정의 (주파수를 이용한 음계 설정)
#define DO_L 523
#define RE 587
#define MI 659
#define FA 698
#define SOL 784
#define RA 880
#define SI 987
#define DO_H 1046

// 메뉴 상수 정의
enum MenuOption { INPUT_PW, SET_PW, CHANGE_PW, SOUND_ON_OFF };  // 메뉴 옵션들
int currentMenu = INPUT_PW;  // 현재 선택된 메뉴 항목
int soundOn = 1;             // 사운드 활성화 여부 (1 = 켜짐, 0 = 꺼짐)

// 비밀번호 변수들
char password[4];            // 설정된 비밀번호
char inputPassword[4];       // 입력된 비밀번호
int inputIndex = 0;          // 비밀번호 입력 인덱스
int isPasswordSet = 0;       // 비밀번호 설정 여부 (1 = 설정됨)
int attempts = 0;            // 비밀번호 틀린 횟수
int changePwAttempt = 0;     // Change PW 메뉴의 시도 여부 확인

// LCD 핸들러
int lcdHandle;

// 부저 및 서보 모터 초기화 및 제어 함수
void Change_FREQ(unsigned int freq) {
    if (soundOn) softToneWrite(BUZZER_PIN, freq);  // 부저 주파수 변경
}

void STOP_FREQ(void) {
    softToneWrite(BUZZER_PIN, 0);  // 부저 멈춤
}

void Buzzer_Init(void) {
    softToneCreate(BUZZER_PIN);  // 부저 초기화
    STOP_FREQ();                 // 초기엔 멈춘 상태
}

void Servo_Init(void) {
    softPwmCreate(SERVO_PIN, 0, 200);  // 서보 모터 초기화 (0~200 범위)
}

void Servo_Open(void) {
    softPwmWrite(SERVO_PIN, 25);  // 서보 모터를 90도로 회전
    delay(2000);  // 2초 동안 열린 상태 유지
    softPwmWrite(SERVO_PIN, 15);   // 서보 모터를 원래 상태로 복귀
}

// 경쾌한 사운드 재생 (올바른 비밀번호 입력 시)
void PlayCheerfulSound(void) {
    int melody[] = {DO_H, SOL, MI, SOL, DO_H};  // 재생할 음계 배열
    int length = sizeof(melody) / sizeof(melody[0]);

    for (int i = 0; i < length; i++) {
        Change_FREQ(melody[i]);  // 각각의 음을 재생
        delay(200);
    }
    STOP_FREQ();
}

// 디바운스 함수 (버튼 중복 입력 방지)
int debounce(int pin) {
    if (digitalRead(pin) == LOW) {
        delay(50);
        if (digitalRead(pin) == LOW) {
            while (digitalRead(pin) == LOW);
            return 1;
        }
    }
    return 0;
}

// readKeypad 함수: 키패드 입력을 읽어오는 함수
char readKeypad() {
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
    if (debounce(BUTTON_ENTER_PIN)) return 'E';
    if (debounce(BUTTON_DOWN_PIN)) return 'D';
    return '\0';  // 아무 버튼도 눌리지 않았을 때
}

// LCD 메뉴 표시 함수
void displayMenu() {
    lcdClear(lcdHandle);  // LCD 화면 초기화
    switch (currentMenu) {
        case INPUT_PW:
            lcdPuts(lcdHandle, "1. Input PW");  // 비밀번호 입력
            break;
        case SET_PW:
            lcdPuts(lcdHandle, "2. Set PW");    // 비밀번호 설정
            break;
        case CHANGE_PW:
            lcdPuts(lcdHandle, "3. Change PW"); // 비밀번호 변경
            break;
        case SOUND_ON_OFF:
            lcdPuts(lcdHandle, "4. Sound on/off");  // 사운드 온/오프
            break;
    }
}

// 메뉴 스크롤 함수
void scrollMenu() {
    currentMenu = (currentMenu + 1) % 4;  // 메뉴 순환
    displayMenu();
    Change_FREQ(SOL);  // 메뉴 스크롤 시 소리 재생
    delay(100);
    STOP_FREQ();
}

// 비밀번호 입력 처리 함수 (입력 시 소리 재생)
void processPasswordInput(char key) {
    if (inputIndex < 4) {
        inputPassword[inputIndex++] = key;   // 입력된 키를 배열에 저장
        lcdPosition(lcdHandle, inputIndex - 1, 1);  // '*' 표시
        lcdPutchar(lcdHandle, '*');
        
        // 경쾌한 소리 재생 (입력 시 음계)
        int tones[] = {DO_L, MI, SOL, DO_H};
        Change_FREQ(tones[inputIndex % 4]);
        delay(100);
        STOP_FREQ();
    }
}

// 비밀번호 설정 함수
void setPassword() {
    if (inputIndex == 4) {
        for (int i = 0; i < 4; i++) {
            password[i] = inputPassword[i];  // 입력된 비밀번호를 저장
        }
        isPasswordSet = 1;  // 비밀번호가 설정됨
        lcdClear(lcdHandle);
        lcdPuts(lcdHandle, "Password Set!");
        delay(1000);
        displayMenu();
        inputIndex = 0;
        memset(inputPassword, 0, sizeof(inputPassword));  // 입력 초기화
    }
}

// 비밀번호 확인 함수
void checkPassword() {
    if (strncmp(password, inputPassword, 4) == 0) {  // 비밀번호 일치 여부 확인
        lcdClear(lcdHandle);
        lcdPuts(lcdHandle, "Door opened");
        PlayCheerfulSound();  // 올바른 비밀번호 입력 시 경쾌한 소리 재생
        Servo_Open();         // 도어를 여는 동작
        attempts = 0;         // 시도 횟수 초기화
    } else {
        attempts++;           // 틀린 시도 증가
        lcdClear(lcdHandle);
        lcdPuts(lcdHandle, "Invalid password");
        Change_FREQ(1000);    // 경고음 재생
        delay(1000);
        STOP_FREQ();
        
        if (attempts >= 3) {  // 3회 오류 시 10초간 잠금
            lcdClear(lcdHandle);
            lcdPuts(lcdHandle, "Locked 10 sec");
            Change_FREQ(500);
            delay(10000);
            STOP_FREQ();
            attempts = 0;
        }
    }
    inputIndex = 0;
    memset(inputPassword, 0, sizeof(inputPassword));
}

// 메뉴 기능 실행 함수
void executeMenuOption() {
    lcdClear(lcdHandle);
    Change_FREQ(RE);
    delay(100);
    STOP_FREQ();
    
    switch (currentMenu) {
        case INPUT_PW:  // 비밀번호 입력 메뉴
            if (!isPasswordSet) {  // 비밀번호 미설정 시 안내 메시지
                lcdPuts(lcdHandle, "Set PW first");
                delay(2000);
                displayMenu();
                return;
            }
            lcdPuts(lcdHandle, "Input PW:");
            delay(500);
            inputIndex = 0;
            memset(inputPassword, 0, sizeof(inputPassword));
            while (inputIndex < 4) {
                char key = readKeypad();
                if (key >= '0' && key <= '9') {
                    processPasswordInput(key);
                }
            }
            checkPassword();
            displayMenu();
            break;
        case SET_PW:  // 비밀번호 설정 메뉴
            if (isPasswordSet) {
                lcdPuts(lcdHandle, "Already Set");
                delay(2000);
                displayMenu();
                return;
            }
            lcdPuts(lcdHandle, "Set PW:");
            delay(500);
            while (inputIndex < 4) {
                char key = readKeypad();
                if (key >= '0' && key <= '9') {
                    processPasswordInput(key);
                }
            }
            setPassword();
            break;
        case CHANGE_PW:  // 비밀번호 변경 메뉴
            lcdPuts(lcdHandle, "As is: ");
            delay(500);
            inputIndex = 0;
            memset(inputPassword, 0, sizeof(inputPassword));
            while (inputIndex < 4) {
                char key = readKeypad();
                if (key >= '0' && key <= '9') {
                    processPasswordInput(key);
                }
            }
            if (strncmp(password, inputPassword, 4) == 0) {  // 기존 비밀번호 확인
                lcdClear(lcdHandle);
                lcdPuts(lcdHandle, "To be: ");
                delay(500);
                inputIndex = 0;
                memset(inputPassword, 0, sizeof(inputPassword));
                while (inputIndex < 4) {
                    char key = readKeypad();
                    if (key >= '0' && key <= '9') {
                        processPasswordInput(key);
                    }
                }
                setPassword();
            } else {
                lcdPuts(lcdHandle, "Invalid password");
                delay(1000);
                inputIndex = 0;
                memset(inputPassword, 0, sizeof(inputPassword));
                displayMenu();
            }
            break;
        case SOUND_ON_OFF:  // 사운드 온/오프 메뉴
            soundOn = !soundOn;  // 사운드 상태 변경
            lcdClear(lcdHandle);
            lcdPuts(lcdHandle, soundOn ? "Sound on" : "Sound off");
            delay(1000);
            displayMenu();
            break;
    }
}

int main(void) {
    if (wiringPiSetupGpio() == -1) return 1;

    lcdHandle = lcdInit(2, 16, 4, 2, 4, 20, 21, 12, 16, 0, 0, 0, 0);  // LCD 초기화
    displayMenu();

    // 버튼 핀 초기화
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
    pinMode(BUTTON_DOWN_PIN, INPUT);
    pinMode(BUTTON_ENTER_PIN, INPUT);

    Buzzer_Init();  // 부저 초기화
    Servo_Init();   // 서보 초기화

    while (1) {
        if (debounce(BUTTON_DOWN_PIN)) {  // 아래 버튼 누름
            scrollMenu();
        }
        if (debounce(BUTTON_ENTER_PIN)) {  // 엔터 버튼 누름
            executeMenuOption();
        }
        delay(100);  // 반복 주기
    }

    return 0;
}