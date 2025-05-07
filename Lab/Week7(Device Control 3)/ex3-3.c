#include <stdio.h>
#include <wiringPi.h>
#include <softTone.h>
#include <softPwm.h>
#include <string.h>
#include <lcd.h>  // LCD 제어를 위한 헤더파일

// 핀 정의
#define BUZZER_PIN 17
#define SERVO_PIN 18
#define BUTTON1_PIN 27  
#define BUTTON2_PIN 22 
#define BUTTON3_PIN 10 
#define BUTTON4_PIN 9 
#define BUTTON5_PIN 11  
#define BUTTON6_PIN 5 
#define BUTTON7_PIN 6  
#define BUTTON8_PIN 13  
#define BUTTON9_PIN 19  
#define BUTTON0_PIN 26  
#define BUTTON_DOWN_PIN 23  // ↓ 버튼 (메뉴 스크롤)
#define BUTTON_ENTER_PIN 24  // E 버튼 (비밀번호 설정 완료)

// 음계 주파수 정의
#define DO_L 523
#define RE 587
#define MI 659
#define FA 698
#define SOL 784
#define RA 880
#define SI 987
#define DO_H 1046

// 메뉴 상수
enum MenuOption { INPUT_PW, SET_PW, CHANGE_PW, SOUND_ON_OFF };
int currentMenu = 0;  // 현재 메뉴 화면 (0: 1,2번 메뉴 / 1: 3,4번 메뉴)
int soundOn = 1;  // 사운드 켜짐 상태

// 비밀번호 변수
char password[4];  // 설정된 비밀번호 저장
char inputPassword[4];  // 입력된 비밀번호 저장
int inputIndex = 0;  // 입력된 자리
int isPasswordSet = 0;  // 비밀번호가 설정되었는지 여부
int attempts = 0;  // 시도 횟수

// LCD 핸들러
int lcdHandle;

// 부저 및 서보 모터 제어 함수
void Change_FREQ(unsigned int freq) {
    if (soundOn) softToneWrite(BUZZER_PIN, freq);  // 부저 주파수 변경
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
    int melody[] = {DO_H, SOL, MI, SOL, DO_H};
    int length = sizeof(melody) / sizeof(melody[0]);

    for (int i = 0; i < length; i++) {
        Change_FREQ(melody[i]);  // 각각의 음 재생
        delay(200);
    }
    STOP_FREQ();
}

// 디바운스 함수
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

// LCD 메뉴 표시 함수 (두 줄씩 표시)
void displayMenu() {
    lcdClear(lcdHandle);
    if (currentMenu == 0) {
        lcdPosition(lcdHandle, 0, 0);  // 첫 번째 줄의 첫 번째 칸으로 커서 이동
        lcdPuts(lcdHandle, "1. Input PW");
        lcdPosition(lcdHandle, 0, 1);  // 두 번째 줄의 첫 번째 칸으로 커서 이동
        lcdPuts(lcdHandle, "2. Set PW");
    } else if (currentMenu == 1) {
        lcdPosition(lcdHandle, 0, 0);  // 첫 번째 줄의 첫 번째 칸으로 커서 이동
        lcdPuts(lcdHandle, "3. Change PW");
        lcdPosition(lcdHandle, 0, 1);  // 두 번째 줄의 첫 번째 칸으로 커서 이동
        lcdPuts(lcdHandle, "4. Sound on/off");
    }
}

// 메뉴 스크롤 함수
void scrollMenu() {
    currentMenu = (currentMenu + 1) % 2;  // 0과 1 사이를 순환
    displayMenu();
    Change_FREQ(SOL);  // 메뉴 스크롤 시 소리 재생
    delay(100);
    STOP_FREQ();
}

// 비밀번호 입력 처리 함수 (입력 시 소리 재생)
void processPasswordInput(char key) {
    if (inputIndex < 4) {
        inputPassword[inputIndex++] = key;
        lcdPosition(lcdHandle, inputIndex - 1, 1);
        lcdPutchar(lcdHandle, '*');
        
        // 경쾌한 소리 재생 (숫자 입력 시 음 재생)
        int tones[] = {DO_L, MI, SOL, DO_H};
        Change_FREQ(tones[inputIndex % 4]);
        delay(100);  // 음 재생 시간
        STOP_FREQ();
    }
}

// 비밀번호 설정 함수
void setPassword() {
    if (inputIndex == 4) {
        for (int i = 0; i < 4; i++) {
            password[i] = inputPassword[i];
        }
        isPasswordSet = 1;
        lcdClear(lcdHandle);
        lcdPuts(lcdHandle, "Password Set!");
        delay(1000);
        displayMenu();
        inputIndex = 0;
        memset(inputPassword, 0, sizeof(inputPassword));
    }
}

// 비밀번호 확인 함수
void checkPassword() {
    if (strncmp(password, inputPassword, 4) == 0) {
        lcdClear(lcdHandle);
        lcdPuts(lcdHandle, "Door opened");
        PlayCheerfulSound();
        Servo_Open();
        attempts = 0;
    } else {
        attempts++;
        lcdClear(lcdHandle);
        lcdPuts(lcdHandle, "Invalid password");
        Change_FREQ(1000);
        delay(1000);
        STOP_FREQ();
        
        if (attempts >= 3) {
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

// 메뉴 기능 실행 함수 (각 버튼 번호로 실행)
void executeMenuOption(int menuOption) {
    lcdClear(lcdHandle);
    Change_FREQ(RE);  // Enter 버튼 누를 때 소리 재생
    delay(100);
    STOP_FREQ();
    
    switch (menuOption) {
        case 1:  // Input PW
            if (!isPasswordSet) {
                lcdPuts(lcdHandle, "Set PW first");
                delay(2000);
                displayMenu();
                return;
            }
            lcdPuts(lcdHandle, "Input PW:");
            delay(500);
            inputIndex = 0;  // Input PW 메뉴에 들어올 때 비밀번호 입력 초기화
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
        case 2:  // Set PW
            if (isPasswordSet) {
                lcdPuts(lcdHandle, "Already Set");
                delay(2000);
                displayMenu();
                return;
            }
            lcdPuts(lcdHandle, "Set new PW:");
            delay(500);
            while (inputIndex < 4) {
                char key = readKeypad();
                if (key >= '0' && key <= '9') {
                    processPasswordInput(key);
                }
            }
            setPassword();
            break;
        case 3:  // Change PW
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
            if (strncmp(password, inputPassword, 4) == 0) {
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
                lcdClear(lcdHandle);
                lcdPuts(lcdHandle, "Invalid password");
                delay(1000);
                inputIndex = 0;
                memset(inputPassword, 0, sizeof(inputPassword));
                displayMenu();
            }
            break;
        case 4:  // Sound on/off
            soundOn = !soundOn;
            lcdClear(lcdHandle);
            lcdPuts(lcdHandle, soundOn ? "Sound on" : "Sound off");
            delay(1000);
            displayMenu();
            break;
    }
}

int main(void) {
    if (wiringPiSetupGpio() == -1) return 1;

    lcdHandle = lcdInit(2, 16, 4, 2, 4, 20, 21, 12, 16, 0, 0, 0, 0);
    displayMenu();

    // 버튼 핀 초기화
    pinMode(BUTTON1_PIN, INPUT);
    pinMode(BUTTON2_PIN, INPUT);
    pinMode(BUTTON3_PIN, INPUT);
    pinMode(BUTTON4_PIN, INPUT);
    pinMode(BUTTON0_PIN, INPUT);
    pinMode(BUTTON5_PIN, INPUT);
    pinMode(BUTTON6_PIN, INPUT);
    pinMode(BUTTON7_PIN, INPUT);
    pinMode(BUTTON8_PIN, INPUT);
    pinMode(BUTTON9_PIN, INPUT);
    pinMode(BUTTON_DOWN_PIN, INPUT);
    pinMode(BUTTON_ENTER_PIN, INPUT);

    Buzzer_Init();
    Servo_Init();

    while (1) {
        if (debounce(BUTTON_DOWN_PIN)) {  // ↓ 버튼 눌러 메뉴 스크롤
            scrollMenu();
        }
        if (debounce(BUTTON1_PIN)) {  // 1번 버튼 누르면 Input PW 실행
            executeMenuOption(1);
        }
        if (debounce(BUTTON2_PIN)) {  // 2번 버튼 누르면 Set PW 실행
            executeMenuOption(2);
        }
        if (debounce(BUTTON3_PIN)) {  // 3번 버튼 누르면 Change PW 실행
            executeMenuOption(3);
        }
        if (debounce(BUTTON4_PIN)) {  // 4번 버튼 누르면 Sound on/off 실행
            executeMenuOption(4);
        }
        delay(100);
    }

    return 0;
}