#include <wiringPi.h>  // GPIO 제어를 위한 WiringPi 라이브러리 헤더 파일 포함
#include <softPwm.h>   // 소프트웨어 PWM 제어를 위한 SoftPWM 라이브러리 헤더 파일 포함

// 모터 핀 정의
#define MOTOR_MT_P_PIN 4   // 모터의 양극 핀
#define MOTOR_MT_N_PIN 17  // 모터의 음극 핀

// LED 핀 정의
#define LED_PIN_1 5
#define LED_PIN_2 6
#define LED_PIN_3 13

// 버튼 핀 정의
#define BUTTON1_PIN 23  // 모터 정지 버튼
#define BUTTON2_PIN 24  // 약한 속도 버튼
#define BUTTON3_PIN 25  // 중간 속도 버튼
#define BUTTON4_PIN 26  // 강한 속도 버튼
#define BUTTON5_PIN 27  // 회전 방향 전환 버튼

// 모터 회전 방향 정의
#define RIGHT_ROTATE 1  // 오른쪽 회전
#define LEFT_ROTATE 2   // 왼쪽 회전

// 전역 변수 설정
int rotateDirection = RIGHT_ROTATE; // 초기 회전 방향 (오른쪽)
int currentSpeed = 0;               // 현재 속도 (초기 0)

// 함수 프로토타입 선언
void MotorStop(void); // 모터 정지 함수
void MotorControl(unsigned char speed, unsigned char rotate); // 모터 제어 함수
void updateLED(int speedLevel);  // 속도에 따라 LED를 제어하는 함수

int main(void) {
    // GPIO 초기화
    if(wiringPiSetupGpio() == -1)
        return 1; // 초기화 실패 시 종료
    
    // 모터 핀 설정
    pinMode(MOTOR_MT_N_PIN, OUTPUT);
    pinMode(MOTOR_MT_P_PIN, OUTPUT);

    // 버튼 핀 설정 및 풀업 저항 활성화
    pinMode(BUTTON1_PIN, INPUT);
    pinMode(BUTTON2_PIN, INPUT);
    pinMode(BUTTON3_PIN, INPUT);
    pinMode(BUTTON4_PIN, INPUT);
    pinMode(BUTTON5_PIN, INPUT);
    
    pullUpDnControl(BUTTON1_PIN, PUD_UP);
    pullUpDnControl(BUTTON2_PIN, PUD_UP);
    pullUpDnControl(BUTTON3_PIN, PUD_UP);
    pullUpDnControl(BUTTON4_PIN, PUD_UP);
    pullUpDnControl(BUTTON5_PIN, PUD_UP);

    // LED 핀 설정
    pinMode(LED_PIN_1, OUTPUT);
    pinMode(LED_PIN_2, OUTPUT);
    pinMode(LED_PIN_3, OUTPUT);
    
    // 소프트 PWM 생성
    softPwmCreate(MOTOR_MT_N_PIN, 0, 100);
    softPwmCreate(MOTOR_MT_P_PIN, 0, 100);

    while(1) {
        if(digitalRead(BUTTON1_PIN) == LOW) { // 버튼 1: 모터 정지
            MotorStop();
            updateLED(0);
            currentSpeed = 0;
        }
        else if(digitalRead(BUTTON2_PIN) == LOW) { // 버튼 2: 약한 속도로 오른쪽 회전
            currentSpeed = 30;
            rotateDirection = RIGHT_ROTATE;
            MotorControl(currentSpeed, rotateDirection);
            updateLED(1);
        }
        else if(digitalRead(BUTTON3_PIN) == LOW) { // 버튼 3: 중간 속도로 오른쪽 회전
            currentSpeed = 60;
            rotateDirection = RIGHT_ROTATE;
            MotorControl(currentSpeed, rotateDirection);
            updateLED(2);
        }
        else if(digitalRead(BUTTON4_PIN) == LOW) { // 버튼 4: 강한 속도로 오른쪽 회전
            currentSpeed = 100;
            rotateDirection = RIGHT_ROTATE;
            MotorControl(currentSpeed, rotateDirection);
            updateLED(3);
        }
        else if(digitalRead(BUTTON5_PIN) == LOW) { // 버튼 5: 회전 방향 전환 (현재 속도 유지)
            rotateDirection = (rotateDirection == RIGHT_ROTATE) ? LEFT_ROTATE : RIGHT_ROTATE;
            MotorStop();
            delay(50);  // 반대 방향 전환을 위한 짧은 지연
            MotorControl(currentSpeed, rotateDirection);
            delay(500); // 버튼 디바운싱 시간
        }
        
        delay(100); // 상태 체크 주기
    }

    return 0;
}

// 모터 정지 함수: 모터 양극과 음극 핀의 PWM 값을 0으로 설정하여 모터 정지
void MotorStop() {
    softPwmWrite(MOTOR_MT_N_PIN, 0);
    softPwmWrite(MOTOR_MT_P_PIN, 0);
}

// 모터 제어 함수: 모터 속도와 방향에 따라 제어
void MotorControl(unsigned char speed, unsigned char rotate) {
    if(rotate == LEFT_ROTATE) {          // 좌회전 설정
        digitalWrite(MOTOR_MT_P_PIN, LOW);            // P 핀 LOW
        softPwmWrite(MOTOR_MT_N_PIN, speed);          // N 핀에 속도 설정
    }
    else if(rotate == RIGHT_ROTATE) {    // 우회전 설정
        digitalWrite(MOTOR_MT_N_PIN, LOW);            // N 핀 LOW
        softPwmWrite(MOTOR_MT_P_PIN, speed);          // P 핀에 속도 설정
    }
}

// 속도에 따라 LED 점등 함수: 속도 레벨에 따라 LED를 켜거나 끔
void updateLED(int speedLevel) {
    // 속도 레벨에 따라 LED 점등
    digitalWrite(LED_PIN_1, (speedLevel >= 1) ? HIGH : LOW); // 속도 레벨이 1 이상일 때 LED_PIN_1 켜기
    digitalWrite(LED_PIN_2, (speedLevel >= 2) ? HIGH : LOW); // 속도 레벨이 2 이상일 때 LED_PIN_2 켜기
    digitalWrite(LED_PIN_3, (speedLevel >= 3) ? HIGH : LOW); // 속도 레벨이 3 이상일 때 LED_PIN_3 켜기
}