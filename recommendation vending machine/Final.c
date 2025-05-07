#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <errno.h>
#include <lcd.h>
#include <stdint.h>
#include <stdlib.h>
#include <softPwm.h>


// 음료 구조체 정의
typedef struct {
    char name[50];  // 음료 이름
    int isCold;     // 1: 차가운 음료, 0: 따뜻한 음료
    int mood;       // 1: 상쾌, 2: 평온, 3: 피곤
    int taste;      // 1: 달콤한 맛, 2: 쓴 맛, 3: 상관없음
    int caffeine;   // 1: 낮(카페인 포함), 0: 밤(카페인 제외)
    int price;      // 음료 가격
    int stock;      // 재고 수량
    int isSoldOut;  // 품절 상태: 1이면 품절, 0이면 재고 있음
    int servoPin;   // 음료별 서보 모터 핀 번호
} Drink;


// 함수 원형 선언
void selectDrink(Drink drinks[]); // 음료 선택 함수
void updateDrinkStock(Drink* drink); // 음료 재고 업데이트 함수
void paymentSystem(Drink* selectedDrink, Drink drinks[]); // 결제 시스템 함수
void displayMainMenu(); // 초기 메뉴 화면 표시 함수
char readKeypad();
void adminMenu(Drink drinks[]);
void addMachineBalance();
void cashPaymentSystem(Drink* selectedDrink, Drink drinks[]);
void enterAdminMode(Drink drinks[], int* prevState);
int isHomeAndEnterLongPressed();
void moveCursor(int row, int col);
void playInputSound();
void playSuccessSound();
void playFailureSound();
void MotorControl(unsigned char speed, unsigned char rotate);
void MotorStopGradual();
void MotorStop();
void setupMotorPins();
void playBuzzer(int frequency, int duration);


//1. 센서 및 핀 정의
//전역 변수
int machineBalance = 10000;  // 자판기 초기 잔고
#define ADMIN_PASSWORD "2443"  // 관리자 비밀번호

#define CS_MCP3208 8  // SPI 칩 선택 핀
#define SPI_CHANNEL 0
#define SPI_SPEED 1000000

// 키패드 핀 정의
#define ROW1 5   // 행 1
#define ROW2 6   // 행 2
#define ROW3 4   // 행 3
#define ROW4 17  // 행 4

#define COL1 27  // 열 1
#define COL2 22  // 열 2
#define COL3 16  // 열 3
#define COL4 20  // 열 4


// 버저 핀 번호
#define BUZZER_PIN 18

// 모터 핀 정의
#define MOTOR_PWM_PIN 19  // PWM 핀
#define MOTOR_MT_P_PIN 23   // 모터의 양극 핀
#define MOTOR_MT_N_PIN 24   // 모터의 음극 핀

// 모터 회전 방향 정의
#define FORWARD 1  // 정방향 회전
#define REVERSE 2  // 역방향 회전

// 온습도 센서 핀 정의
#define MAXTIMINGS 83
#define DHTPIN 26

// PCF8591의 I2C 주소
#define PCF8591_ADDR 0x48 

int prevState; // 전역 변수로 선언

// 온습도 센서 데이터 저장 배열
int dht11_dat[5] = {0};

//2. 입력 처리 및 유틸리티
//메시지 출력 위치 및 내용을 저장하는 구조체 정의
typedef struct {
    int row;           // 출력 행
    int col;           // 출력 열
    char message[100]; // 출력할 메시지
} Message;

//메시지 출력 함수(덮어씌우는 문제 방지)
void printMessageStruct(Message msg) {
    moveCursor(msg.row, msg.col);   // 지정된 위치로 커서 이동
    printf("\033[K");               // 현재 커서 위치부터 줄 끝까지 삭제
    printf("%s", msg.message);      // 메시지 출력
    fflush(stdout);                 // 출력 강제 갱신
}

// ANSI 화면 초기화 함수
void clearScreen() {
    printf("\033[2J"); // 화면 전체 지우기
    printf("\033[H");  // 커서를 화면 맨 위로 이동
}

// ANSI 커서 이동 함수  결제 시스템에서 사용자 입력받는 화면이 계속 깜빡이는 문제를 해결하기 위해
//입력된 값이나 동적인 부분만 갱신하도록 변경
void moveCursor(int row, int col) {
    printf("\033[%d;%dH", row, col); // row행 col열로 커서 이동
}

//초기 출력
void displayMainMenu() {
    clearScreen();
    printf("음료 자판기에 오신 것을 환영합니다!\n");
    printf("잔고: %d원\n", machineBalance);
    printf("1번: 사용자 선택 / 2번: 음료 추천 시스템\n");
    fflush(stdout);
}

// 음료 목록 표시
void displayDrinks(Drink drinks[], int startIdx, int endIdx) {
    printf("\n음료 목록 (페이지 %d):\n", (startIdx / 5) + 1);
    for (int i = startIdx; i <= endIdx && i < 40; i++) {
        printf("%d. %s\n", i + 1, drinks[i].name);
    }

}

// 키패드 배열
char keys[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'H', '0', 'E', 'D'}
};

// 행과 열 핀 배열
int rowPins[4] = {ROW1, ROW2, ROW3, ROW4};
int colPins[4] = {COL1, COL2, COL3, COL4};

// 디바운스 함수
int debouncePin(int pin) {
    if (digitalRead(pin) == HIGH) {  // 첫 번째 HIGH 감지
        delay(50);  // 50ms 동안 안정화 대기
        if (digitalRead(pin) == HIGH) {  // 다시 확인
            while (digitalRead(pin) == HIGH);  // 키가 떼어질 때까지 대기
            delay(50);  // 떼어진 후 다시 안정화
            return 1;  // 안정적으로 눌림 확인
        }
    }
    return 0;
}

// 키패드 읽기 함수
char readKeypad() {
    char keyPressed = '\0';

    for (int row = 0; row < 4; row++) {
        digitalWrite(rowPins[row], HIGH);  // 해당 행 활성화
        delayMicroseconds(100);  // 안정화 대기

        for (int col = 0; col < 4; col++) {
            if (digitalRead(colPins[col]) == HIGH) {  // 키가 눌렸는지 확인
                delay(50);  // 디바운싱
                if (digitalRead(colPins[col]) == HIGH) {  // 안정된 상태 확인
                    keyPressed = keys[row][col];  // 눌린 키 값
                    while (digitalRead(colPins[col]) == HIGH);  // 키가 떼어질 때까지 대기
                    delay(50);  // 안정화 대기
                    digitalWrite(rowPins[row], LOW);  // 행 비활성화
                    return keyPressed;  // 키 반환
                }
            }
        }
        digitalWrite(rowPins[row], LOW);  // 행 비활성화
    }
    return '\0';  // 키 입력이 없으면 NULL 반환
}

// 키패드 핀 모드 설정 함수
void setupKeypadPins() {
    // 행 핀을 출력으로 설정
    for (int i = 0; i < 4; i++) {
        pinMode(rowPins[i], OUTPUT);
        digitalWrite(rowPins[i], LOW); // 초기 상태 LOW
    }

    // 열 핀을 입력으로 설정 (Pull-down 저항 활성화)
    for (int i = 0; i < 4; i++) {
        pinMode(colPins[i], INPUT);
        pullUpDnControl(colPins[i], PUD_DOWN); // 풀다운 저항
    }
}

//관리자 모드 진입 확인
int isHomeAndEnterLongPressed() {
    int duration = 0;
    int hPressed = 0, ePressed = 0;

    // 두 키를 1.5초 동안 동시에 눌렀는지 확인
    while (1) {
        hPressed = 0;
        ePressed = 0;

        // 키 상태 확인
        for (int row = 0; row < 4; row++) {
            digitalWrite(rowPins[row], HIGH);
            delayMicroseconds(100);

            for (int col = 0; col < 4; col++) {
                if (digitalRead(colPins[col]) == HIGH) {
                    char currentKey = keys[row][col];
                    if (currentKey == 'H') hPressed = 1;
                    if (currentKey == 'E') ePressed = 1;
                }
            }
            digitalWrite(rowPins[row], LOW);
        }

        if (hPressed && ePressed) {
            delay(100);  // 디바운싱
            duration += 100;

            if (duration >= 1500) {
                printf("\n관리자 모드 진입 조건 충족. 두 키를 떼세요.\n");
                fflush(stdout);

                // 키가 눌린 상태가 끝나길 기다림
                while (hPressed || ePressed) {
                    hPressed = 0;
                    ePressed = 0;

                    for (int row = 0; row < 4; row++) {
                        digitalWrite(rowPins[row], HIGH);
                        delayMicroseconds(100);

                        for (int col = 0; col < 4; col++) {
                            if (digitalRead(colPins[col]) == HIGH) {
                                char currentKey = keys[row][col];
                                if (currentKey == 'H') hPressed = 1;
                                if (currentKey == 'E') ePressed = 1;
                            }
                        }
                        digitalWrite(rowPins[row], LOW);
                    }
                    delay(50);  // 안정화 대기
                }

                printf("\n관리자 모드로 진입합니다.\n");
                fflush(stdout);
                return 1;
            }
        } else {
            duration = 0;  // 키가 하나라도 떼어진 경우 초기화
        }

        if (!hPressed && !ePressed) break;
    }
    return 0;
}


//3. 음료 관련
// 음료 데이터 초기화 함수
void initializeDrinks(Drink drinks[]) {

    // 차가운 음료 초기화
    strcpy(drinks[0].name, "아이스 아메리카노");
    drinks[0].isCold = 1; drinks[0].mood = 3; drinks[0].taste = 2; drinks[0].caffeine = 1, drinks[0].price = 1500, drinks[0].stock = 1, drinks[0].isSoldOut = 0, drinks[0].servoPin = 12; // 서보 모터 핀

    strcpy(drinks[1].name, "레모네이드");
    drinks[1].isCold = 1; drinks[1].mood = 1; drinks[1].taste = 1; drinks[1].caffeine = 0, drinks[1].price = 1700, drinks[1].stock = 1, drinks[1].isSoldOut = 0, drinks[1].servoPin = 21; // 서보 모터 핀

    strcpy(drinks[2].name, "콜라");
    drinks[2].isCold = 1; drinks[2].mood = 1; drinks[2].taste = 3; drinks[2].caffeine = 1, drinks[2].price = 2000, drinks[2].stock = 1, drinks[2].isSoldOut = 0, drinks[2].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[3].name, "제로콜라");
    drinks[3].isCold = 1; drinks[3].mood = 1; drinks[3].taste = 3; drinks[3].caffeine = 1, drinks[3].price = 2000, drinks[3].stock = 1, drinks[3].isSoldOut = 0, drinks[3].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[4].name, "탄산수");
    drinks[4].isCold = 1; drinks[4].mood = 1; drinks[4].taste = 3; drinks[4].caffeine = 0, drinks[4].price = 2000, drinks[4].stock = 1, drinks[4].isSoldOut = 0, drinks[4].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[5].name, "포도주스");
    drinks[5].isCold = 1; drinks[5].mood = 2; drinks[5].taste = 1; drinks[5].caffeine = 0, drinks[5].price = 1700, drinks[5].stock = 1, drinks[5].isSoldOut = 0, drinks[5].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[6].name, "오렌지주스");
    drinks[6].isCold = 1; drinks[6].mood = 2; drinks[6].taste = 1; drinks[6].caffeine = 0, drinks[6].price = 1700, drinks[6].stock = 1, drinks[6].isSoldOut = 0, drinks[6].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[7].name, "망고주스");
    drinks[7].isCold = 1; drinks[7].mood = 2; drinks[7].taste = 1; drinks[7].caffeine = 0, drinks[7].price = 1700, drinks[7].stock = 1, drinks[7].isSoldOut = 0, drinks[7].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[8].name, "수박주스");
    drinks[8].isCold = 1; drinks[8].mood = 2; drinks[8].taste = 1; drinks[8].caffeine = 0, drinks[8].price = 1700, drinks[8].stock = 1, drinks[8].isSoldOut = 0, drinks[8].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[9].name, "민트티");
    drinks[9].isCold = 1; drinks[9].mood = 1; drinks[9].taste = 2; drinks[9].caffeine = 0, drinks[9].price = 1500, drinks[9].stock = 1, drinks[9].isSoldOut = 0, drinks[9].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[10].name, "솔의눈");
    drinks[10].isCold = 1; drinks[10].mood = 1; drinks[10].taste = 3; drinks[10].caffeine = 0, drinks[10].price = 1700, drinks[10].stock = 1, drinks[10].isSoldOut = 0, drinks[10].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[11].name, "아이스티");
    drinks[11].isCold = 1; drinks[11].mood = 2; drinks[11].taste = 3; drinks[11].caffeine = 0, drinks[11].price = 1500, drinks[11].stock = 1, drinks[11].isSoldOut = 0, drinks[11].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[12].name, "사이다");
    drinks[12].isCold = 1; drinks[12].mood = 1; drinks[12].taste = 3; drinks[12].caffeine = 0, drinks[12].price = 2000, drinks[12].stock = 1, drinks[12].isSoldOut = 0, drinks[12].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[13].name, "환타");
    drinks[13].isCold = 1; drinks[13].mood = 1; drinks[13].taste = 3; drinks[13].caffeine = 0, drinks[13].price = 2000, drinks[13].stock = 1, drinks[13].isSoldOut = 0, drinks[13].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[14].name, "헛개차");
    drinks[14].isCold = 1; drinks[14].mood = 3; drinks[14].taste = 3; drinks[14].caffeine = 0, drinks[14].price = 1500, drinks[14].stock = 1, drinks[14].isSoldOut = 0, drinks[14].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[15].name, "이온음료");
    drinks[15].isCold = 1; drinks[15].mood = 2; drinks[15].taste = 3; drinks[15].caffeine = 0, drinks[15].price = 1700, drinks[15].stock = 1, drinks[15].isSoldOut = 0, drinks[15].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[16].name, "아침햇살");
    drinks[16].isCold = 1; drinks[16].mood = 2; drinks[16].taste = 3; drinks[16].caffeine = 0, drinks[16].price = 1700, drinks[16].stock = 1, drinks[16].isSoldOut = 0, drinks[16].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[17].name, "하늘보리");
    drinks[17].isCold = 1; drinks[17].mood = 2; drinks[17].taste = 2; drinks[17].caffeine = 0, drinks[17].price = 1500, drinks[17].stock = 1, drinks[17].isSoldOut = 0, drinks[17].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[18].name, "아이스초코");
    drinks[18].isCold = 1; drinks[18].mood = 3; drinks[18].taste = 1; drinks[18].caffeine = 0, drinks[18].price = 1500, drinks[18].stock = 1, drinks[18].isSoldOut = 0, drinks[18].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[19].name, "미숫가루");
    drinks[19].isCold = 1; drinks[19].mood = 2; drinks[19].taste = 3; drinks[19].caffeine = 0, drinks[19].price = 1500, drinks[19].stock = 1, drinks[19].isSoldOut = 0, drinks[19].servoPin = 21;  // 기본값으로 비활성 핀 지정

    // 따뜻한 음료 초기화
    strcpy(drinks[20].name, "고구마라떼");
    drinks[20].isCold = 0; drinks[20].mood = 3; drinks[20].taste = 1; drinks[20].caffeine = 0, drinks[20].price = 1500, drinks[20].stock = 1, drinks[20].isSoldOut = 0, drinks[20].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[21].name, "생강차");
    drinks[21].isCold = 0; drinks[21].mood = 3; drinks[21].taste = 3; drinks[21].caffeine = 0, drinks[21].price = 1500, drinks[21].stock = 1, drinks[21].isSoldOut = 0, drinks[21].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[22].name, "대추차");
    drinks[22].isCold = 0; drinks[22].mood = 3; drinks[22].taste = 3; drinks[22].caffeine = 0, drinks[22].price = 1500, drinks[22].stock = 1, drinks[22].isSoldOut = 0, drinks[22].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[23].name, "유자차");
    drinks[23].isCold = 0; drinks[23].mood = 2; drinks[23].taste = 1; drinks[23].caffeine = 0, drinks[23].price = 1500, drinks[23].stock = 1, drinks[23].isSoldOut = 0, drinks[23].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[24].name, "꿀물");
    drinks[24].isCold = 0; drinks[24].mood = 3; drinks[24].taste = 1; drinks[24].caffeine = 0, drinks[24].price = 1500, drinks[24].stock = 1, drinks[24].isSoldOut = 0, drinks[24].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[25].name, "바닐라라떼");
    drinks[25].isCold = 0; drinks[25].mood = 3; drinks[25].taste = 1; drinks[25].caffeine = 1, drinks[25].price = 1500, drinks[25].stock = 1, drinks[25].isSoldOut = 0, drinks[25].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[26].name, "카라멜마키아토");
    drinks[26].isCold = 0; drinks[26].mood = 2; drinks[26].taste = 1; drinks[26].caffeine = 1, drinks[26].price = 1500, drinks[26].stock = 1, drinks[26].isSoldOut = 0, drinks[26].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[27].name, "핫초코");
    drinks[27].isCold = 0; drinks[27].mood = 3; drinks[27].taste = 1; drinks[27].caffeine = 0, drinks[27].price = 1500, drinks[27].stock = 1, drinks[27].isSoldOut = 0, drinks[27].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[28].name, "두유");
    drinks[28].isCold = 0; drinks[28].mood = 2; drinks[28].taste = 3; drinks[28].caffeine = 0, drinks[28].price = 1500, drinks[28].stock = 1, drinks[28].isSoldOut = 0, drinks[28].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[29].name, "홍차");
    drinks[29].isCold = 0; drinks[29].mood = 2; drinks[29].taste = 2; drinks[29].caffeine = 1, drinks[29].price = 1500, drinks[29].stock = 1, drinks[29].isSoldOut = 0, drinks[29].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[30].name, "율무차");
    drinks[30].isCold = 0; drinks[30].mood = 2; drinks[30].taste = 3; drinks[30].caffeine = 0, drinks[30].price = 1500, drinks[30].stock = 1, drinks[30].isSoldOut = 0, drinks[30].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[31].name, "밀크커피");
    drinks[31].isCold = 0; drinks[31].mood = 3; drinks[31].taste = 2; drinks[31].caffeine = 1, drinks[31].price = 1500, drinks[31].stock = 1, drinks[31].isSoldOut = 0, drinks[31].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[32].name, "밀크티");
    drinks[32].isCold = 0; drinks[32].mood = 2; drinks[32].taste = 3; drinks[32].caffeine = 1, drinks[32].price = 1500, drinks[32].stock = 1, drinks[32].isSoldOut = 0, drinks[32].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[33].name, "레쓰비");
    drinks[33].isCold = 0; drinks[33].mood = 3; drinks[33].taste = 2; drinks[33].caffeine = 1, drinks[33].price = 1500, drinks[33].stock = 1, drinks[33].isSoldOut = 0, drinks[33].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[34].name, "도라지차");
    drinks[34].isCold = 0; drinks[34].mood = 3; drinks[34].taste = 3; drinks[34].caffeine = 0, drinks[34].price = 1500, drinks[34].stock = 1, drinks[34].isSoldOut = 0, drinks[34].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[35].name, "레몬티");
    drinks[35].isCold = 0; drinks[35].mood = 1; drinks[35].taste = 3; drinks[35].caffeine = 0, drinks[35].price = 1500, drinks[35].stock = 1, drinks[35].isSoldOut = 0, drinks[35].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[36].name, "녹차라떼");
    drinks[36].isCold = 0; drinks[36].mood = 2; drinks[36].taste = 2; drinks[36].caffeine = 1, drinks[36].price = 1500, drinks[36].stock = 1, drinks[36].isSoldOut = 0, drinks[36].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[37].name, "곡물라떼");
    drinks[37].isCold = 0; drinks[37].mood = 2; drinks[37].taste = 1; drinks[37].caffeine = 0, drinks[37].price = 1500, drinks[37].stock = 1, drinks[37].isSoldOut = 0, drinks[37].servoPin = 21;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[38].name, "인삼차");
    drinks[38].isCold = 0; drinks[38].mood = 1; drinks[38].taste = 2; drinks[38].caffeine = 0, drinks[38].price = 1500, drinks[38].stock = 1, drinks[38].isSoldOut = 0, drinks[38].servoPin = 12;  // 기본값으로 비활성 핀 지정

    strcpy(drinks[39].name, "쌍화차");
    drinks[39].isCold = 0; drinks[39].mood = 3; drinks[39].taste = 3; drinks[39].caffeine = 0, drinks[39].price = 1500, drinks[39].stock = 1, drinks[39].isSoldOut = 0, drinks[39].servoPin = 21;  // 기본값으로 비활성 핀 지정

}

// 음료 선택 및 결제 시스템 연결 
void selectDrink(Drink drinks[]) {
    int currentPage = 0;
    int drinksPerPage = 5;  // 한 페이지에 표시할 음료 수
    int totalDrinks = 40;   // 총 음료 수
    int drinkNumber = 0;    // 선택된 음료 번호

    while (1) {
        clearScreen();
        int startIdx = currentPage * drinksPerPage;
        int endIdx = startIdx + drinksPerPage - 1;
        if (endIdx >= totalDrinks) endIdx = totalDrinks - 1;

        printf("\n음료 목록 (페이지 %d):\n", currentPage + 1);
        for (int i = startIdx; i <= endIdx; i++) {
            printf("%d. %s %s\n", i + 1, drinks[i].name, (drinks[i].isSoldOut ? "(품절)" : ""));
        }
        printf("\n음료 번호 입력 (D: 다음 페이지, H: 홈): ");
        fflush(stdout);

        drinkNumber = 0;  // 초기화
        while (1) {
            char key = readKeypad();
            if (key >= '0' && key <= '9') {
                drinkNumber = drinkNumber * 10 + (key - '0');
                printf("%c", key);
                fflush(stdout);
            } else if (key == 'E') {
                printf("\n디버깅: 입력된 음료 번호 = %d\n", drinkNumber);
                if (drinkNumber >= startIdx + 1 && drinkNumber <= endIdx + 1) {
                    if (drinks[drinkNumber - 1].isSoldOut) {
                        printf("\n선택한 음료는 품절입니다. 다른 음료를 선택하세요.\n");
                        delay(2000);
                    } else {
                        clearScreen();
                        printf("선택된 음료: %s\n", drinks[drinkNumber - 1].name);
                        paymentSystem(&drinks[drinkNumber - 1], drinks);
                        return;
                    }
                } else {
                    printf("\n잘못된 입력입니다. 현재 페이지의 음료를 선택해주세요.\n");
                    delay(2000);
                }
                break;
            } else if (key == 'D') {
                currentPage = (currentPage + 1) % ((totalDrinks + drinksPerPage - 1) / drinksPerPage);
                break;
            } else if (key == 'H') {
                printf("홈 화면으로 돌아갑니다.\n");
                delay(2000);
                return;
            }
        }
    }
}

//품절 상태
void updateDrinkStock(Drink* drink) {
    drink->stock--;  // 재고 감소
    if (drink->stock <= 0) {
        drink->stock = 0;  // 음수 방지
        drink->isSoldOut = 1;  // 품절 상태 설정
    }
}

//음료 재고 관리 함수
void manageDrinkStock(Drink drinks[]) {
    int currentPage = 0;          // 현재 페이지
    int drinksPerPage = 5;        // 한 페이지에 표시할 음료 수
    int totalDrinks = 40;         // 총 음료 수
    int drinkNumber = 0;          // 선택된 음료 번호

    while (1) {
        clearScreen();
        int startIdx = currentPage * drinksPerPage;
        int endIdx = startIdx + drinksPerPage - 1;
        if (endIdx >= totalDrinks) endIdx = totalDrinks - 1;

        // 현재 음료 목록과 재고 상태 출력
        printf("=== 음료 재고 관리 === (페이지 %d)\n\n", currentPage + 1);
        for (int i = startIdx; i <= endIdx; i++) {
            printf("%d. %s - 재고: %d개 %s\n", 
                i + 1, 
                drinks[i].name, 
                drinks[i].stock, 
                (drinks[i].isSoldOut ? "(품절)" : ""));
        }
        printf("\n입고할 음료 번호를 입력하세요 (D: 다음 페이지, H: 홈 버튼): ");
        fflush(stdout);

        char key;
        drinkNumber = 0;  // 초기화
        while (1) {
            key = readKeypad();
            if (key >= '0' && key <= '9') {
                drinkNumber = drinkNumber * 10 + (key - '0');
                printf("%c", key);
                fflush(stdout);
            } else if (key == 'E') {  // 엔터 입력 시 확인
                if (drinkNumber >= 1 && drinkNumber <= totalDrinks) {
                    if (drinkNumber - 1 >= startIdx && drinkNumber - 1 <= endIdx) {
                        // 선택된 음료 입고
                        drinks[drinkNumber - 1].stock += 10;
                        drinks[drinkNumber - 1].isSoldOut = 0;  // 품절 해제
                        moveCursor(7 + drinkNumber - startIdx, 1);  // 음료 위치로 이동
                        printf("\033[K");  // 기존 메시지 삭제
                        moveCursor(10,1);
                        printf("%d. %s - 재고: %d개 %s\n", 
                            drinkNumber, 
                            drinks[drinkNumber - 1].name, 
                            drinks[drinkNumber - 1].stock, 
                            (drinks[drinkNumber - 1].isSoldOut ? "(품절)" : ""));
                        fflush(stdout);
                        delay(2000);
                        break;
                    } else {
                        // 선택된 음료가 현재 페이지 범위를 벗어남
                        moveCursor(11, 1);
                        printf("잘못된 음료 번호입니다. 현재 페이지에서 선택해주세요.\n");
                        fflush(stdout);
                        delay(2000);
                        break;
                    }
                } else {
                    moveCursor(11, 1);
                    printf("잘못된 음료 번호입니다. 다시 입력하세요.\n");
                    fflush(stdout);
                    delay(2000);
                    break;
                }
            } else if (key == 'D') {  // 다음 페이지로 이동
                currentPage++;
                if (currentPage * drinksPerPage >= totalDrinks) {
                    currentPage = 0;  // 마지막 페이지 이후 첫 페이지로 돌아감
                }
                break;  // 페이지 전환
            } else if (key == 'H') {  // 홈 버튼 입력 시 관리자 메뉴로 돌아감
                printf("\n관리자 메뉴로 돌아갑니다.\n");
                delay(2000);
                return;
            }
        }
    }
}


//4. 결제 관련
//현금 결제 로직 함수
void cashPaymentSystem(Drink* selectedDrink, Drink drinks[]) {
    char key = '\0';
    int receivedAmount = 0;
    int change = -selectedDrink->price;
    char inputBuffer[6] = "";
    int inputIndex = 0;
    int prevState = 2;  // 현재 상태를 현금 결제 시스템으로 설정

    // 초기 메시지 출력
    Message initialMsg = {1, 1, ""};
    snprintf(initialMsg.message, sizeof(initialMsg.message), "선택된 음료: %s", selectedDrink->name);
    printMessageStruct(initialMsg);

    Message priceMsg = {2, 1, ""};
    snprintf(priceMsg.message, sizeof(priceMsg.message), "음료 가격: %d원", selectedDrink->price);
    printMessageStruct(priceMsg);

    Message receivedMsg = {3, 1, "받은 금액: 0원"};
    printMessageStruct(receivedMsg);

    Message changeMsg = {4, 1, ""};
    snprintf(changeMsg.message, sizeof(changeMsg.message), "거스름돈: %d원", change);
    printMessageStruct(changeMsg);

    Message inputPrompt = {6, 1, "현금을 입력하세요 (100원 단위, 최대 5자리): "};
    printMessageStruct(inputPrompt);

    // 버저 핀 초기화
    pinMode(BUZZER_PIN, OUTPUT);

    // 모터 핀 초기화
    int servoPin = selectedDrink->servoPin;
    pinMode(servoPin, OUTPUT);
    softPwmCreate(servoPin, 0, 200);  // 서보 모터 PWM 설정

    while (1) {
        // 관리자 모드 진입 체크
        if (isHomeAndEnterLongPressed()) {
            printf("\n관리자 모드로 진입합니다.\n");
            delay(2000);
            enterAdminMode(drinks, &prevState);  // 현재 상태 전달
            return;
        }

        key = readKeypad(); // 키 입력 확인

        if (key == '\0') {
            continue;
        } else if (key >= '0' && key <= '9') {  // 숫자 입력
            if (inputIndex < 5) {
                inputBuffer[inputIndex++] = key;
                inputBuffer[inputIndex] = '\0'; // Null-terminate
                receivedAmount = atoi(inputBuffer);
                change = receivedAmount - selectedDrink->price;

                playInputSound(); // 입력 소리

                // 받은 금액 및 거스름돈 갱신
                snprintf(receivedMsg.message, sizeof(receivedMsg.message), "받은 금액: %d원", receivedAmount);
                printMessageStruct(receivedMsg);

                snprintf(changeMsg.message, sizeof(changeMsg.message), "거스름돈: %d원", change);
                printMessageStruct(changeMsg);
            }
        } else if (key == 'D') {  // 마지막 입력 삭제
            if (inputIndex > 0) {
                inputBuffer[--inputIndex] = '\0';  // 입력 버퍼에서 마지막 문자 제거

                // 입력 영역 초기화
                moveCursor(6, 1);  // "현금을 입력하세요..." 줄 위치로 이동
                printf("현금을 입력하세요 (100원 단위, 최대 5자리):     ");  // 초기화된 메시지 출력
                fflush(stdout);

                // 현재 입력값 출력
                if (inputIndex > 0) {
                    moveCursor(6, 45);  // 숫자를 출력할 위치로 이동
                    printf("%s", inputBuffer);
                } else {
                    moveCursor(6, 45);  // 입력값 없는 경우 빈 칸으로 초기화
                    printf("     ");
                }
                fflush(stdout);

                // 받은 금액 및 거스름돈 계산
                receivedAmount = (inputIndex > 0) ? atoi(inputBuffer) : 0;
                change = receivedAmount - selectedDrink->price;

                // 받은 금액 및 거스름돈 메시지 업데이트
                snprintf(receivedMsg.message, sizeof(receivedMsg.message), "받은 금액: %d원", receivedAmount);
                printMessageStruct(receivedMsg);

                snprintf(changeMsg.message, sizeof(changeMsg.message), "거스름돈: %d원", change);
                printMessageStruct(changeMsg);
            }
        } else if (key == 'E') {  // Enter 키 처리
            // 금액 입력 후 정방향 회전
            MotorControl(50, FORWARD);
            delay(1000);  // 모터가 동작하는 동안 대기
            MotorStop();

            if (receivedAmount >= selectedDrink->price) {
                change = receivedAmount - selectedDrink->price;

                if (change > machineBalance) {
                    // 잔돈 부족 메시지 출력 및 초기화
                    Message errorMsg = {8, 1, "거스름돈이 부족합니다. 잔고를 충전해주세요."};
                    printMessageStruct(errorMsg);
                    playFailureSound(); // 실패 소리

                    // 반대 방향으로 회전
                    MotorControl(50, REVERSE);
                    delay(1000);
                    MotorStop();

                    delay(1500);

                    // 메시지 초기화
                    snprintf(errorMsg.message, sizeof(errorMsg.message), "");
                    printMessageStruct(errorMsg);

                    memset(inputBuffer, 0, sizeof(inputBuffer));
                    inputIndex = 0;
                    receivedAmount = 0;
                    change = -selectedDrink->price;

                    // 받은 금액 및 거스름돈 초기화
                    snprintf(receivedMsg.message, sizeof(receivedMsg.message), "받은 금액: 0원");
                    printMessageStruct(receivedMsg);

                    snprintf(changeMsg.message, sizeof(changeMsg.message), "거스름돈: %d원", change);
                    printMessageStruct(changeMsg);

                    continue;
                }

                // 결제 성공 메시지 출력
                Message successMsg = {8, 1, "결제가 완료되었습니다! 음료를 제공 중입니다."};
                printMessageStruct(successMsg);
                playSuccessSound(); // 성공 소리

                // 서보 모터 동작
                if (servoPin > 0) {
                    softPwmWrite(servoPin, 5); // 서보모터 동작
                    delay(2000);
                    softPwmWrite(servoPin, 15); // 원래 위치로 복귀
                }

                // 잔돈 반환 로직
                if (change > 0) {
                    MotorControl(50, REVERSE);  // 잔돈 반환
                    delay(1000);
                    MotorStop();
                }

                // 재고 업데이트
                updateDrinkStock(selectedDrink);
                machineBalance -= change;               // 잔돈 차감
                delay(2000);
                return;
            } else {
                // 금액 부족 메시지 출력 및 초기화
                Message insufficientMsg = {8, 1, "금액이 부족합니다. 다시 입력해주세요."};
                printMessageStruct(insufficientMsg);
                playFailureSound(); // 실패 소리

                // 반대 방향으로 회전
                MotorControl(50, REVERSE);
                delay(1000);
                MotorStop();

                delay(1500);

                // 메시지 초기화
                snprintf(insufficientMsg.message, sizeof(insufficientMsg.message), "");
                printMessageStruct(insufficientMsg);

                memset(inputBuffer, 0, sizeof(inputBuffer));
                inputIndex = 0;
                receivedAmount = 0;
                change = -selectedDrink->price;

                // 받은 금액 및 거스름돈 초기화
                snprintf(receivedMsg.message, sizeof(receivedMsg.message), "받은 금액: 0원");
                printMessageStruct(receivedMsg);

                snprintf(changeMsg.message, sizeof(changeMsg.message), "거스름돈: %d원", change);
                printMessageStruct(changeMsg);
            }
        } else if (key == 'H') {  // 홈 버튼 처리
            Message homeMsg = {8, 1, "홈 화면으로 돌아갑니다."};
            printMessageStruct(homeMsg);
            delay(2000);

            // 메시지 초기화
            snprintf(homeMsg.message, sizeof(homeMsg.message), "");
            printMessageStruct(homeMsg);

            return;
        }
    }
}


// Python 스크립트를 실행하는 함수
int executeRFIDScript() {
    int ret = system("python3 cardread.py");  // Python 스크립트 실행
    if (ret != 0) {
        printf("RFID 태그 읽기 실패: Python 스크립트 오류.\n");
        return 0;  // 실패 반환
    }

    // Python 스크립트가 정상 실행된 경우 RFID 데이터를 확인
    FILE* file = fopen("rfid_data.txt", "r");
    if (file == NULL) {
        printf("Error: rfid_data.txt 파일을 열 수 없습니다.\n");
        return 0;
    }

    char id[20];
    if (fgets(id, sizeof(id), file) != NULL) {
        printf("RFID 태그 ID: %s\n", id);
        fclose(file);
        return 1;  // 성공 반환
    } else {
        printf("파일에서 RFID ID를 읽을 수 없습니다.\n");
        fclose(file);
        return 0;  // 실패 반환
    }
}

// 카드 결제 로직 함수
void cardPaymentSystem(Drink* selectedDrink) {
    clearScreen();
    Message rfidMsg = {1, 1, "카드를 리더기에 대주세요..."};  // 카드 리더 메시지
    printMessageStruct(rfidMsg);

    // 서보 모터 초기화
    int servoPin = selectedDrink->servoPin; // 선택된 음료의 서보 핀
    pinMode(servoPin, OUTPUT);
    softPwmCreate(servoPin, 0, 200); // 선택된 서보 모터 PWM 설정

    // **카드 인식 대기 소리 (삐빅)**
    playBuzzer(1000, 100); // 1000Hz, 100ms
    delay(50);            // 200ms 대기
    playBuzzer(1000, 100); // 두 번째 삐빅

    // RFID 태그 읽기 시도
    int cardReadSuccess = executeRFIDScript();

    if (cardReadSuccess) {
        // 결제 성공
        Message successMsg = {9, 1, "카드 결제가 완료되었습니다."};
        printMessageStruct(successMsg);


        // **결제 완료 소리**
        playSuccessSound(); // 1500Hz, 200ms

        // 서보 모터 동작
        if (selectedDrink->servoPin > 0) {
            softPwmWrite(selectedDrink->servoPin, 5); // 서보모터 동작
            delay(2000);
            softPwmWrite(selectedDrink->servoPin, 15); // 원래 위치로 복귀
        } else {
            printf("\n서보모터 동작이 필요하지 않은 음료입니다.\n");
        }

        // 재고 업데이트
        updateDrinkStock(selectedDrink);

        delay(3000);  // 3초 대기
        } else {

        // **결제 실패 소리**
        playFailureSound(); // 실패 소리 (500Hz, 300ms)

        // 결제 실패
        Message failMsg = {9, 1, "카드 결제가 실패했습니다. 다시 시도해주세요."};
        printMessageStruct(failMsg);
        delay(3000);  // 3초 대기
    }
}

// 결제 시스템 함수
void paymentSystem(Drink* selectedDrink, Drink drinks[]) {
    while (1) {

        // 관리자 모드 진입 체크
        if (isHomeAndEnterLongPressed()) {
            printf("\n관리자 모드로 진입합니다.\n");
            delay(2000);
            enterAdminMode(drinks, &prevState);  // 현재 상태 전달
            return;
        }

        clearScreen(); // 화면 초기화
        Message paymentChoiceMsg = {1, 1, "결제 방식을 선택하세요:\n1. 현금 결제\n2. 카드 결제"};
        printMessageStruct(paymentChoiceMsg);
        fflush(stdout);

        char key = '\0';
        while (key == '\0') {
            key = readKeypad();
        }

        if (key == '1') {
            // 현금 결제
            if (machineBalance < selectedDrink->price) {
                // 잔고 부족 시 메시지 출력
                Message cashMsg = {5, 1, "현금 결제 불가: 자판기 잔고 부족\n"};
                printMessageStruct(cashMsg);
                fflush(stdout);
                delay(2000); // 메시지를 사용자에게 보여주기 위해 대기
                return; // 현금 결제 종료
            } else {
                cashPaymentSystem(selectedDrink, drinks); // 잔고 충분 시 현금 결제 진행
                break;
            }
        } else if (key == '2') {
            // 카드 결제 로직 호출 (RFID 추가 예정)
            cardPaymentSystem(selectedDrink);
            break;  // 결제 완료 후 루프 탈출
        } else if (key == 'H') {
            // 홈으로 돌아가기
            Message homeMsg = {9, 1, "홈 화면으로 돌아갑니다."};
            printMessageStruct(homeMsg);
            delay(3000);  // 3초 대기
            return;
        } else {
            Message invalidChoiceMsg = {9, 1, "잘못된 선택입니다. 다시 시도하세요."};
            printMessageStruct(invalidChoiceMsg);
            delay(2000); // 잘못된 선택 대기
        }
    }
}


//5. 관리자 모드 관련
//관리자 모드 진입 함수
void enterAdminMode(Drink drinks[], int* prevState) {
    char inputPassword[5] = "";
    int inputIndex = 0;

    clearScreen();
    printf("관리자 모드 진입\n비밀번호를 입력하세요: ");
    fflush(stdout);

    while (1) {
        char key = readKeypad();

        if (key >= '0' && key <= '9' && inputIndex < 4) {
            inputPassword[inputIndex++] = key;
            printf("*"); // 비밀번호 입력 표시
            fflush(stdout);
        } else if (key == 'E') {  // 엔터 입력
            inputPassword[inputIndex] = '\0';
            if (strcmp(inputPassword, ADMIN_PASSWORD) == 0) {
                printf("\n비밀번호 확인 완료. 관리자 모드로 진입합니다.\n");
                delay(2000);

                // 관리자 메뉴 호출
                adminMenu(drinks);

                // 관리자 모드 종료 후 이전 상태로 복귀
                if (*prevState == 1) {
                    selectDrink(drinks);  // 음료 선택 메뉴로 복귀
                } else if (*prevState == 2) {
                    cashPaymentSystem(drinks + 0, drinks);  // 현금 결제 시스템으로 복귀
                } else {
                    displayMainMenu(); // 초기 메뉴로 복귀
                }
                return;
            } else {
                // 비밀번호 틀렸을 때 처리
                printf("\n비밀번호가 틀렸습니다. 다시 시도하세요.\n");
                delay(1500);

                // 메시지 및 입력 초기화
                moveCursor(1, 1); // 화면 상단으로 이동
                printf("\033[J"); // 화면 지우기 (현재 줄 아래 모두 삭제)
                printf("관리자 모드 진입\n비밀번호를 입력하세요: ");
                fflush(stdout);

                // 입력값 초기화
                inputIndex = 0;
                memset(inputPassword, 0, sizeof(inputPassword));
            }
        } else if (key == 'H') {  // 홈 버튼으로 관리자 모드 종료
            printf("\n관리자 모드를 종료합니다.\n");
            delay(2000);

            // 이전 상태로 복귀
            if (*prevState == 1) {
                selectDrink(drinks);
            } else if (*prevState == 2) {
                cashPaymentSystem(drinks + 0, drinks);
            }
            return;
        }
    }
}

//관리자 모드 메뉴
void adminMenu(Drink drinks[]) {
    while (1) {
        clearScreen();
        printf("=== 관리자 모드 ===\n");
        printf("1. 잔고 채우기\n");
        printf("2. 음료 재고 관리\n");
        printf("H. 관리자 모드 종료\n");
        printf("선택: ");
        fflush(stdout);

        char key = '\0';
        while (key == '\0') {  // 입력이 있을 때까지 대기
            key = readKeypad();  // 키 입력 읽기
        }

        if (key == '1') {
            addMachineBalance();  // 잔고 채우기 함수 호출
        } else if (key == '2') {
            manageDrinkStock(drinks);  // 음료 관리 함수 호출
        } else if (key == 'H') {
            printf("관리자 모드를 종료합니다.\n");
            delay(2000);
            return;
        } else {
            printf("잘못된 입력입니다. 다시 선택해주세요.\n");
            delay(2000);
        }
    }
}

//잔고 채우기 함수
void addMachineBalance() {
    int inputAmount = 0;
    char inputBuffer[6] = "";
    int inputIndex = 0;

    clearScreen();
    printf("현재 잔고: %d원\n", machineBalance);
    printf("충전할 금액을 입력하세요 (100원 단위): ");
    fflush(stdout);

    while (1) {
        char key = readKeypad();

        if (key >= '0' && key <= '9' && inputIndex < 5) {
            inputBuffer[inputIndex++] = key;
            printf("%c", key);
            fflush(stdout);
        } else if (key == 'E') {  // 엔터 입력
            inputAmount = atoi(inputBuffer);
            if (inputAmount % 100 == 0 && inputAmount > 0) {
                machineBalance += inputAmount;
                moveCursor(3, 1);  // 결과 출력 위치
                printf("잔고가 %d원으로 충전되었습니다.\n", machineBalance);
                fflush(stdout);
                delay(2000);
                return;
            } else {
                // 메시지 및 입력 초기화
                moveCursor(3, 1);  // "충전할 금액을 입력하세요" 아래로 이동
                printf("\033[J");  // 현재 커서 아래 모든 내용 삭제
                printf("올바르지 않은 금액입니다. 100원 단위로 다시 입력해주세요.\n");
                fflush(stdout);
                delay(2000);

                // 입력 초기화
                moveCursor(2, 1);  // 다시 입력 위치로 이동
                printf("\033[J");  // 메시지 초기화
                printf("충전할 금액을 입력하세요 (100원 단위): ");
                fflush(stdout);
                memset(inputBuffer, 0, sizeof(inputBuffer));
                inputIndex = 0;
            }
        } else if (key == 'H') {
            printf("\n관리자 메뉴로 돌아갑니다.\n");
            delay(2000);
            return;
        }
    }
}


//6. 추천 및 환경 센서 관련
// 음료 추천 로직 함수
void recommendDrink(Drink drinks[], int isCold, int caffeine, int mood, int taste) {
    while (1) {
        clearScreen();
        printf("추천 음료 목록:\n");
        int count = 0;
        int indices[40];  // 추천 음료의 원래 인덱스를 저장할 배열

        for (int i = 0; i < 40; i++) {
            if (drinks[i].isCold == isCold &&
                drinks[i].mood == mood &&
                drinks[i].caffeine == caffeine &&
                (taste == 3 || drinks[i].taste == taste) &&
                drinks[i].stock > 0) {
                indices[count++] = i;
                printf("%d. %s (가격: %d원, 재고: %d개)\n",
                    count, drinks[i].name, drinks[i].price, drinks[i].stock);
            }
        }

        if (count == 0) {
            printf("조건에 맞는 음료가 없습니다.\n");
            delay(3000);
            return;
        }

        printf("\n음료 번호를 입력하세요. (H: 홈으로 돌아가기): ");
        fflush(stdout);

        int drinkNumber = 0;
        while (1) {
            char key = readKeypad();
            if (key >= '0' && key <= '9') {
                drinkNumber = drinkNumber * 10 + (key - '0');
                printf("%c", key);
                fflush(stdout);
            } else if (key == 'E') {
                if (drinkNumber >= 1 && drinkNumber <= count) {
                    int selectedIndex = indices[drinkNumber - 1];
                    clearScreen();
                    printf("선택된 음료: %s\n", drinks[selectedIndex].name);
                    paymentSystem(&drinks[selectedIndex], drinks);
                    return;
                } else {
                    printf("\n잘못된 음료 번호입니다.\n");
                    delay(2000);
                    break;
                }
            } else if (key == 'H') {
                clearScreen();
                printf("홈 화면으로 돌아갑니다.\n");
                return;
            }
        }
    }
}


// 날씨 선택 후 기분 상태와 CDS 센서를 통한 추천 시스템
void handleDrinkRecommendation(Drink drinks[]) {
    while (1) {
        char key = readKeypad();
        if (key == '1' || key == '2') {  // 차가운 음료(1) 또는 따뜻한 음료(2)
            int isCold = (key == '1') ? 1 : 0;

            // 1. 기분 상태 선택
            clearScreen();
            printf("기분 상태를 선택하세요:\n");
            printf("1. 상쾌한 기분\n");
            printf("2. 평온한 기분\n");
            printf("3. 피곤한 상태\n");
            fflush(stdout);

            int mood = 0;
            while (1) {  // 기분 선택 루프
                char moodKey = readKeypad();
                if (moodKey >= '1' && moodKey <= '3') {
                    mood = moodKey - '0';
                    break;
                } else if (moodKey != '\0') {
                    printf("잘못된 입력입니다. 1, 2, 3 중에서 선택해주세요.\n");
                    fflush(stdout);
                }
            }

            // 2. 맛 선호도 선택
            clearScreen();
            printf("맛 선호도를 선택하세요:\n");
            printf("1. 달콤한 맛\n");
            printf("2. 쓴 맛\n");
            printf("3. 상관없음\n");
            fflush(stdout);

            int taste = 0;
            while (1) {  // 맛 선택 루프
                char tasteKey = readKeypad();
                if (tasteKey >= '1' && tasteKey <= '3') {
                    taste = tasteKey - '0';
                    break;
                } else if (tasteKey != '\0') {
                    printf("잘못된 입력입니다. 1, 2, 3 중에서 선택해주세요.\n");
                    fflush(stdout);
                }
            }

            // 3. 조도 센서로 낮/밤 판별
            // PCF8591 초기화
            int fd = wiringPiI2CSetup(PCF8591_ADDR);  // I2C 주소 설정
            if (fd < 0) {
                fprintf(stderr, "PCF8591 초기화 실패\n");
                return;
            }

            // CDS 센서 값 읽기
            wiringPiI2CWrite(fd, 0x00);  // AIN0 채널 선택
            wiringPiI2CRead(fd);         // 첫 번째 더미 읽기
            int cdsValue = wiringPiI2CRead(fd);  // 실제 데이터 읽기

            // 낮/밤 판별 메시지 출력
            int caffeine = 0;  // 카페인 포함 여부
            clearScreen();
            if (cdsValue > 100) {  // 값이 높으면 빛이 약한 상태
                printf("CDS 센서 값: %d\n", cdsValue);
                printf("편안한 밤입니다. 카페인이 없는 음료를 추천합니다!\n");
                caffeine = 0;  // 밤에는 카페인 없는 음료
            } else {
                printf("CDS 센서 값: %d\n", cdsValue);
                printf("활동량이 많은 낮이네요! 카페인이 들어있는 음료를 추천합니다!\n");
                caffeine = 1;  // 낮에는 카페인 포함 음료
            }
            delay(2000);  // 2초 대기

            // 4. 음료 추천 호출
            recommendDrink(drinks, isCold, caffeine, mood, taste);
            break;
        } else if (key == 'H') {  // 홈 버튼 입력
            clearScreen();
            printf("홈 화면으로 돌아갑니다.\n");
            return;
        } else if (key != '\0') {
            printf("잘못된 입력입니다. 1 또는 2를 선택해주세요.\n");
            fflush(stdout);
        }
    }
}

// `read_dht11_dat` 함수
void read_dht11_dat(Drink drinks[]) {
    clearScreen();
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    // 센서에 시작 신호 보내기
    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);

    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(30);
    pinMode(DHTPIN, INPUT);

    // 센서로부터 데이터 읽기
    for (i = 0; i < MAXTIMINGS; i++) {
        counter = 0;
        while (digitalRead(DHTPIN) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 200) break;
        }

        laststate = digitalRead(DHTPIN);

        if (counter == 200) break;
        if ((i >= 4) && (i % 2 == 0)) {
            dht11_dat[j / 8] <<= 1;
            if (counter > 50) dht11_dat[j / 8] |= 1;
            j++;
        }
    }

    // 데이터 유효성 검사 및 출력
    if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xff))) {
        int temp = dht11_dat[2];
        printf("지금의 습도는 %d.%d%%, 온도는 %d.%d°C네요!\n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
        

        // 온도에 따른 안내 메시지 출력
        if (temp >= 25 && temp <= 35) {
            printf("날씨가 더운데 시원한 음료를 드시려면 1번\n 뜨거운 음료를 드시려면 2번을 눌러주세요.\n1번 시원한 음료 / 2번 뜨거운 음료");
        } else if (temp >= 10 && temp < 25) {
            printf("선선한 가을 날씨네요! 차가운 음료는 1번\n 뜨거운 음료는 2번을 눌러주세요.\n1번 시원한 음료 / 2번 뜨거운 음료");
        } else if (temp >= 0 && temp < 10) {
            printf("조금 쌀쌀한 날씨에요! 뜨거운 음료는 1번\n 차가운 음료는 2번을 눌러주세요.\n1번 시원한 음료 / 2번 뜨거운 음료");
        } else if (temp >= -20 && temp < 0) {
            printf("날씨가 너무 춥네요! 뜨거운 음료는 1번\n 차가운 음료는 2번을 눌러주세요.\n1번 시원한 음료 / 2번 뜨거운 음료");
        }

        fflush(stdout);
        handleDrinkRecommendation(drinks);  // 사용자 선택 처리
    } else {
        printf("온습도 데이터를 읽는 데 실패했습니다. 다시 시도해주세요.\n");
    }
}




//7. 하드웨어 제어 관련

void setupMotorPins() {
    pinMode(MOTOR_PWM_PIN, OUTPUT);
    pinMode(MOTOR_MT_P_PIN, OUTPUT);
    pinMode(MOTOR_MT_N_PIN, OUTPUT);

    // 초기 상태 정지 설정
    MotorStop();
}

//모터 제어 함수(Stop, StopGradual, rotateMotor)
void MotorStop() {
    softPwmWrite(MOTOR_PWM_PIN, 0);  // PWM 신호 끄기
    softPwmStop(MOTOR_PWM_PIN);
    digitalWrite(MOTOR_MT_N_PIN, LOW); // 음극 핀 LOW
    digitalWrite(MOTOR_MT_P_PIN, LOW); // 양극 핀 LOW
}


// 점진적으로 모터를 멈추는 함수
void MotorStopGradual() {
    for (int speed = 100; speed >= 0; speed -= 10) { // 속도를 점진적으로 줄임
        softPwmWrite(MOTOR_PWM_PIN, speed);
        delay(500); // 100ms 간격으로 감소
    }
    MotorStop(); // 완전히 멈춤
}


void MotorControl(unsigned char speed, unsigned char rotate) {
    softPwmWrite(MOTOR_PWM_PIN, speed);  // PWM 속도 설정

    if (rotate == FORWARD) {
        digitalWrite(MOTOR_MT_P_PIN, HIGH);
        digitalWrite(MOTOR_MT_N_PIN, LOW);
    } else if (rotate == REVERSE) {
        digitalWrite(MOTOR_MT_P_PIN, LOW);
        digitalWrite(MOTOR_MT_N_PIN, HIGH);
    } else {  // 정지 상태 처리
        MotorStop();
    }
}


// 특정 주파수와 지속 시간으로 버저를 울리는 함수
void playBuzzer(int frequency, int duration) {
    int period = 1000000 / frequency; // 주파수의 주기 (마이크로초)
    int halfPeriod = period / 2; // 반주기 (HIGH와 LOW의 길이)

    for (int i = 0; i < (duration * 1000) / period; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(halfPeriod);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(halfPeriod);
    }
}

// 입력 소리
void playInputSound() {
    playBuzzer(1000, 100); // 1000Hz, 100ms
}

// 성공 소리
void playSuccessSound() {
    playBuzzer(1500, 200); // 1500Hz, 200ms
}

// 실패 소리
void playFailureSound() {
    playBuzzer(500, 300); // 500Hz, 300ms
}

//main 함수
int main() {
    if (wiringPiSetupGpio() == -1) {
        printf("GPIO 초기화 실패\n");
        return 1;
    }

    // 키패드 핀 설정
    setupKeypadPins();

    setupMotorPins();


    // 음료 초기화
    Drink drinks[40];
    initializeDrinks(drinks);

    // 자판기 잔고 초기화
    machineBalance = 100000;

    int prevState = 0;  // 0: 초기 메뉴, 1: 음료 선택, 2: 현금 결제

    while (1) {
        clearScreen();  // 화면 초기화
        displayMainMenu();  // 초기 메뉴 화면 표시
        fflush(stdout);

        // 관리자 모드 진입 체크
        // printf("디버깅: 관리자 모드 진입 체크 중...\n");
        if (isHomeAndEnterLongPressed()) {
            prevState = 0;  // 초기 상태로 설정
            fflush(stdout);
            enterAdminMode(drinks, &prevState);  // 관리자 모드로 진입
            continue;  // 관리자 모드 종료 후 메인 루프로 복귀
        }

        // 잔고 확인 및 경고 메시지 출력
        if (machineBalance < 10000) {
            printf("현금결제X\n");
            fflush(stdout);
            delay(1000);  // 사용자에게 메시지 보여주기 위한 딜레이
        }

        // 사용자 입력 대기
        char key = '\0';
        while (key == '\0') {
            key = readKeypad();  // 키패드 입력 읽기
        }

        // 사용자 입력 처리
        if (key == '1') {
            prevState = 1;  // 현재 상태 저장
            selectDrink(drinks);  // 음료 선택 화면으로 이동
        } else if (key == '2') {
            prevState = 0;  // 초기 상태로 복귀
            read_dht11_dat(drinks);  // 온습도 데이터를 읽고 추천 시스템 실행
        } 
        else {
            moveCursor(5, 1);
            printf("잘못된 입력입니다. 1 또는 2를 선택해주세요.");
            fflush(stdout);
            delay(2000);
        }
    }
    return 0;
}

