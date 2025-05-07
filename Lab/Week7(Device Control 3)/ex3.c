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

// 음계 주파수 정의
#define DO_L 523
#define RE 587
#define MI 659
#define FA 698
#define SOL 784
#define LA 880
#define SI 987
#define DO_H 1046
#define HIGH_RE 1175
#define HIGH_MI 1319
#define LA_SHARP 466
#define SCROLL_TONE 494

// 메뉴 상수
enum MenuOption { INPUT_PW, SET_PW, CHANGE_PW, SOUND_ON_OFF };
int currentMenu = 0;  // 현재 메뉴 화면 (0: 첫 번째 메뉴)
int soundOn = 1;  // 사운드 켜짐 상태

// 비밀번호 변수
char password[4];  // 설정된 비밀번호 저장
char inputPassword[4];  // 입력된 비밀번호 저장
int inputIndex = 0;  // 입력된 자리
int isPasswordSet = 0;  // 비밀번호가 설정되었는지 여부
int attempts = 0;  // 시도 횟수 (1번 메뉴에서만 사용)

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
    int melody[] = { DO_H, SOL, MI, SOL, DO_H };
    int length = sizeof(melody) / sizeof(melody[0]);

    for (int i = 0; i < length; i++) {
        Change_FREQ(melody[i]);  // 각각의 음 재생
        delay(200);
    }
    STOP_FREQ();
}

// 디바운스 함수 -> 누름 유지시 계속된 입력 방지를 위해
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

// 음 재생 함수
void playSoundForKey(char key) {
    int frequency = 0;
    switch (key) {
    case '0': frequency = DO_L; break;
    case '1': frequency = RE; break;
    case '2': frequency = MI; break;
    case '3': frequency = FA; break;
    case '4': frequency = SOL; break;
    case '5': frequency = LA; break;
    case '6': frequency = SI; break;
    case '7': frequency = DO_H; break;
    case '8': frequency = HIGH_RE; break;
    case '9': frequency = HIGH_MI; break;
    case 'E': frequency = LA_SHARP; break;
    case 'D': frequency = SCROLL_TONE; break;
    default: return;  // 매칭되지 않으면 재생하지 않음
    }
    if (frequency > 0) {
        Change_FREQ(frequency);
        delay(100);  // 음 재생 시간
        STOP_FREQ();
    }
}

// LCD 메뉴 표시 함수 (두 줄씩 표시, 순환)
void displayMenu() {
    lcdClear(lcdHandle);
    lcdPosition(lcdHandle, 0, 0);
    switch (currentMenu) {
    case 0:
        lcdPuts(lcdHandle, "1. Input PW");
        lcdPosition(lcdHandle, 0, 1);
        lcdPuts(lcdHandle, "2. Set PW      d");
        break;
    case 1:
        lcdPuts(lcdHandle, "2. Set PW");
        lcdPosition(lcdHandle, 0, 1);
        lcdPuts(lcdHandle, "3. Change PW   d");
        break;
    case 2:
        lcdPuts(lcdHandle, "3. Change PW");
        lcdPosition(lcdHandle, 0, 1);
        lcdPuts(lcdHandle, "4. Sound on/off ");
        break;
    }
}

// 메뉴 스크롤 함수
void scrollMenu() {
    currentMenu = (currentMenu + 1) % 3;
    displayMenu();
    playSoundForKey('D');  // 스크롤 버튼 고유의 음 재생
}

/*
// LCD 메뉴 표시 함수 (두 줄씩 표시, 순환)
void displayMenu() {
    lcdClear(lcdHandle);

    // 첫 번째 줄 표시
    lcdPosition(lcdHandle, 0, 0);  // 첫 번째 줄의 첫 번째 칸으로 커서 이동
    lcdPuts(lcdHandle, currentMenu % 4 == 0 ? "1. Input PW" :
        currentMenu % 4 == 1 ? "2. Set PW" :
        currentMenu % 4 == 2 ? "3. Change PW" :
        "4. Sound on/off");

    // 두 번째 줄 표시
    lcdPosition(lcdHandle, 0, 1);  // 두 번째 줄의 첫 번째 칸으로 커서 이동
    lcdPuts(lcdHandle, (currentMenu + 1) % 4 == 0 ? "1. Input PW  d" :
        (currentMenu + 1) % 4 == 1 ? "2. Set PW  d" :
        (currentMenu + 1) % 4 == 2 ? "3. Change PW  d" :
        "4. Sound on/off");
}
// 메뉴 스크롤 함수
void scrollMenu() {
    currentMenu = (currentMenu + 1) % 4;  // 0~3 사이를 순환
    displayMenu();
    playSoundForMenu(6);  // 스크롤 버튼 고유의 음 재생
}
*/


// 비밀번호 입력 처리 함수 (각 버튼 고유의 음으로 재생)
void processPasswordInput(char key, int offset) {
    if (inputIndex < 4) {
        inputPassword[inputIndex++] = key;

        // LCD에 "*" 표시
        lcdPosition(lcdHandle, offset + inputIndex - 1, 1);  // 입력 위치를 offset으로 조정  As is가 지워지는 문제를 해결하기 위해 As is: **** 정상적인 출력을 위해
        lcdPutchar(lcdHandle, '*');   

        playSoundForKey(key);  // 각 키의 고유 음 재생

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
int checkPassword(int limitAttempts) {
    if (strncmp(password, inputPassword, 4) == 0) {
        attempts = 0;
        return 1;
    } else {
        if (limitAttempts) 
            attempts++;
        
        lcdClear(lcdHandle);
        lcdPuts(lcdHandle, "Invalid password");
        Change_FREQ(1000);
        delay(1000);
        STOP_FREQ();

        // 3회 틀렸을 경우
        if (limitAttempts && attempts >= 3) {
            lcdClear(lcdHandle);
            lcdPuts(lcdHandle, "Locked 10 sec");
            Change_FREQ(500);
            delay(10000);  // 10초 잠금
            STOP_FREQ();
            attempts = 0;   // 시도 횟수 초기화

            // 메뉴 화면으로 복귀
            inputIndex = 0;
            memset(inputPassword, 0, sizeof(inputPassword)); // 입력 초기화
            displayMenu();  // 메뉴 화면으로 복귀
            return -1;  // 실패 후 메뉴로 돌아갔음을 알림
        }

        // 입력을 초기화하여 다시 시도하게 함
        inputIndex = 0;
        memset(inputPassword, 0, sizeof(inputPassword));
        return 0;
    }
}

// 메뉴 기능 실행 함수 (각 버튼 번호로 실행)
void executeMenuOption(int menuOption) {
    lcdClear(lcdHandle);

    switch (menuOption) {
    case 1:  // Input PW
        playSoundForKey('1');
        if (!isPasswordSet) {
            lcdPuts(lcdHandle, "Set PW first!");
            delay(2000);
            displayMenu();
            return;
        }
        lcdPuts(lcdHandle, "Input PW:");
        delay(500);
        inputIndex = 0;
        memset(inputPassword, 0, sizeof(inputPassword));

        // 비밀번호 입력 및 검증 루프
        while (inputIndex < 4) {
            char key = readKeypad();
            if (key >= '0' && key <= '9') {
            processPasswordInput(key, 0);  // Input PW의 경우 offset을 0으로
            }
        }
        
        // 비밀번호 확인
        int result = checkPassword(1);
        if (result == 1) {  // 성공 시
            lcdClear(lcdHandle);
            lcdPuts(lcdHandle, "Door opened");
            PlayCheerfulSound();
            Servo_Open();
            displayMenu();
        } else if (result == 0) {
            executeMenuOption(1);  // 실패 시 다시 비밀번호 입력
        }
        // result가 -1이면 메뉴로 복귀
        break;

    case 2:  // Set PW
        playSoundForKey('2');
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
            processPasswordInput(key, 0);  // Input PW의 경우 offset을 0으로
            }
        }
        setPassword();
        break;

        case 3:  // Change PW
        playSoundForKey('3');
        if (!isPasswordSet) {
            lcdPuts(lcdHandle, "Set PW first!");
            delay(2000);
            displayMenu();
            return;
        }

        int changePWStep = 0;
        inputIndex = 0;
        memset(inputPassword, 0, sizeof(inputPassword));

        while (1) {
            if (changePWStep == 0) {
                lcdClear(lcdHandle);
                lcdPuts(lcdHandle, "3. Change PW");
                lcdPosition(lcdHandle, 0, 1);
                lcdPuts(lcdHandle, "As is: ");
                while (inputIndex < 4) {
                    char key = readKeypad();
                    if (key >= '0' && key <= '9') {
                        processPasswordInput(key, 7);  // "As is: " 뒤에 * 표시
                    }
                }
                if (checkPassword(0)) {
                    changePWStep = 1;
                    inputIndex = 0;
                    memset(inputPassword, 0, sizeof(inputPassword));
                } else {
                    lcdClear(lcdHandle);
                    lcdPuts(lcdHandle, "Invalid password");
                    inputIndex = 0;
                    memset(inputPassword, 0, sizeof(inputPassword));
                    delay(1000);
                    continue;
                }
            } else if (changePWStep == 1) {
                lcdClear(lcdHandle);
                lcdPuts(lcdHandle, "As is: ****");
                lcdPosition(lcdHandle, 0, 1);
                lcdPuts(lcdHandle, "To be: ");
                while (inputIndex < 4) {
                    char key = readKeypad();
                    if (key >= '0' && key <= '9') {
                        processPasswordInput(key, 7);  // "To be: " 뒤에 * 표시
                    }
                }
                setPassword();
                break;
            }
        }
        displayMenu();
        break;

        case 4:  // Sound on/off
            playSoundForKey('4');
            soundOn = !soundOn;
            lcdClear(lcdHandle);
            lcdPuts(lcdHandle, soundOn ? "Sound on" : "Sound off");
            delay(1000);
            displayMenu();
            break;
        }
}

// 메인 함수
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
        if (debounce(BUTTON_DOWN_PIN)) {
            scrollMenu();
        }
        if (debounce(BUTTON1_PIN)) {
            executeMenuOption(1);
        }
        if (debounce(BUTTON2_PIN)) {
            executeMenuOption(2);
        }
        if (debounce(BUTTON3_PIN)) {
            executeMenuOption(3);
        }
        if (debounce(BUTTON4_PIN)) {
            executeMenuOption(4);
        }
        delay(100);
    }

    return 0;
}