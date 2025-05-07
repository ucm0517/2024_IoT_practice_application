#include <wiringPi.h> // GPIO 제어를 위한 WiringPi 라이브러리 헤더 파일 포함
#include <softPwm.h>  // 소프트웨어 PWM 제어를 위한 SoftPWM 라이브러리 헤더 파일 포함

// 모터 핀 정의
#define MOTOR_MT_P_PIN 4   // 모터의 양극 핀
#define MOTOR_MT_N_PIN 17  // 모터의 음극 핀

// 모터 회전 방향 정의
#define LEFT_ROTATE 1  // 좌회전
#define RIGHT_ROTATE 2 // 우회전

// 함수 프로토타입 선언
void MotorStop(void); // 모터 정지 함수
void MotorControl(unsigned char speed, unsigned char rotate); // 모터 제어 함수

int main(void)
{
    // GPIO 초기화
    if(wiringPiSetupGpio() == -1) // wiringPi GPIO 모드를 사용
        return 1; // 초기화 실패 시 종료

    // 모터 핀을 출력 모드로 설정
    pinMode(MOTOR_MT_N_PIN, OUTPUT);
    pinMode(MOTOR_MT_P_PIN, OUTPUT);

    // 소프트웨어 PWM 설정: PWM 범위는 0 ~ 100으로 설정
    softPwmCreate(MOTOR_MT_N_PIN, 0, 100); // MOTOR_MT_N_PIN 핀을 PWM 핀으로 생성
    softPwmCreate(MOTOR_MT_P_PIN, 0, 100); // MOTOR_MT_P_PIN 핀을 PWM 핀으로 생성

    while(1) {
        MotorControl(30, LEFT_ROTATE); // 모터를 30% 속도로 좌회전
        delay(2000);                   // 2초 동안 좌회전 유지
        MotorStop();                   // 모터 정지
        delay(5000);                   // 5초 대기
        MotorControl(80, RIGHT_ROTATE); // 모터를 80% 속도로 우회전
        delay(2000);                   // 2초 동안 우회전 유지
        MotorStop();                   // 모터 정지
        delay(5000);                   // 5초 대기
    }
    return 0;
}

// 모터 정지 함수: 모터 양극과 음극 핀의 PWM 값을 0으로 설정하여 모터 정지
void MotorStop()
{
    softPwmWrite(MOTOR_MT_N_PIN, 0); // MOTOR_MT_N_PIN 핀의 PWM 신호를 0으로 설정
    softPwmWrite(MOTOR_MT_P_PIN, 0); // MOTOR_MT_P_PIN 핀의 PWM 신호를 0으로 설정
}

// 모터 제어 함수: 속도와 방향을 매개변수로 받아 모터를 회전
void MotorControl(unsigned char speed, unsigned char rotate)
{
    if(rotate == LEFT_ROTATE) {       // 좌회전일 경우
        digitalWrite(MOTOR_MT_P_PIN, LOW);   // 모터 양극 핀은 LOW로 설정하여 회전 방향 결정
        softPwmWrite(MOTOR_MT_N_PIN, speed); // 음극 핀에 PWM 신호를 설정하여 속도 조절
    }
    else if(rotate == RIGHT_ROTATE) { // 우회전일 경우
        softPwmWrite(MOTOR_MT_P_PIN, speed); // 양극 핀에 PWM 신호를 설정하여 속도 조절
        digitalWrite(MOTOR_MT_N_PIN, LOW);   // 모터 음극 핀은 LOW로 설정하여 회전 방향 결정
    }
}