#include <wiringPi.h>  // GPIO 핀 제어를 위한 헤더파일

// FND 선택 핀 정의
#define FND_SEL_S0 11
#define FND_SEL_S1 5
#define FND_SEL_S2 6
#define FND_SEL_S3 13
#define FND_SEL_S4 19
#define FND_SEL_S5 26

// FND 데이터 핀 정의
#define FND_DB_A 2
#define FND_DB_B 3
#define FND_DB_C 4
#define FND_DB_D 17
#define FND_DB_E 27
#define FND_DB_F 22
#define FND_DB_G 10
#define FND_DB_DP 9

// 각 비트 정의
#define A_BIT 0x01
#define B_BIT 0x02
#define C_BIT 0x04
#define D_BIT 0x08
#define E_BIT 0x10
#define F_BIT 0x20
#define G_BIT 0x40
#define DP_BIT 0x80

#define FND_DB_PIN_NUM 8
#define MAX_FND_POSITION 6
#define MAX_CHAR 16

const int FndSel[MAX_FND_POSITION] = {
    FND_SEL_S0, FND_SEL_S1, FND_SEL_S2, 
    FND_SEL_S3, FND_SEL_S4, FND_SEL_S5
};

// 숫자와 문자를 7-Segment로 표현하기 위한 테이블
const int FndNumberTable[16] = {
    A_BIT | B_BIT | C_BIT | D_BIT | E_BIT | F_BIT,         // '0'   0x3F
    B_BIT | C_BIT,                                         // '1'   0x06
    A_BIT | B_BIT | D_BIT | E_BIT | G_BIT,                 // '2'   
    A_BIT | B_BIT | C_BIT | D_BIT | G_BIT,                 // '3'
    B_BIT | C_BIT | F_BIT | G_BIT,                         // '4'
    A_BIT | C_BIT | D_BIT | F_BIT | G_BIT,                 // '5'
    A_BIT | C_BIT | D_BIT | E_BIT | F_BIT | G_BIT,         // '6'
    A_BIT | B_BIT | C_BIT,                                 // '7'
    A_BIT | B_BIT | C_BIT | D_BIT | E_BIT | F_BIT | G_BIT, // '8'
    A_BIT | B_BIT | C_BIT | D_BIT | F_BIT | G_BIT,         // '9'
    A_BIT | B_BIT | C_BIT | E_BIT | F_BIT | G_BIT,         // 'A'
    C_BIT | D_BIT | E_BIT | F_BIT | G_BIT,                 // 'b'
    A_BIT | D_BIT | E_BIT | F_BIT,                         // 'C'
    B_BIT | C_BIT | D_BIT | E_BIT | G_BIT,                 // 'd'
    A_BIT | D_BIT | E_BIT | F_BIT | G_BIT,                 // 'E'
    A_BIT | E_BIT | F_BIT | G_BIT                          // 'F'
};

// FND 데이터 핀 배열
const int FndPinTable[8] = {
    FND_DB_A, FND_DB_B, FND_DB_C, FND_DB_D, 
    FND_DB_E, FND_DB_F, FND_DB_G, FND_DB_DP
};

int main (void) {
    if (wiringPiSetupGpio() == -1) return 1;  // GPIO 설정 실패 시 종료

    int i, k;
    int m = 0;

    pinMode(FND_SEL_S0, OUTPUT);  // FND 선택 핀 출력 모드 설정

    for(i = 0; i < MAX_FND_POSITION; i++) {
        pinMode(FndSel[i], OUTPUT);  // FND 데이터 핀 출력 모드 설정
    }
    
    for (i = 0; i<FND_DB_PIN_NUM; i++) {
        pinMode(FndPinTable[i], OUTPUT);
    }

    while(1) 
    {
        for(i = 0; i < MAX_FND_POSITION; i++) {
            digitalWrite(FndSel[i], HIGH);  // FND 위치 초기화
        }

        digitalWrite(FndSel[m % 6], LOW);
        m++;

        for(k = 0; k < MAX_CHAR; k++) {
            for(i = 0; i < FND_DB_PIN_NUM; i++) {
                if(FndNumberTable[k] & (1 << i)) {
                    digitalWrite(FndPinTable[i], HIGH);  // 핀 HIGH로 설정하여 표시
                } else {
                    digitalWrite(FndPinTable[i], LOW);   // 핀 LOW로 설정하여 표시 끄기
                }
            }
            delay(500);  // 500ms 딜레이
        }
    }

    return 0;
}