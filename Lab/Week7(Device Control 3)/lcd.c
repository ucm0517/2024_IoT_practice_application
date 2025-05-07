#include <stdio.h>
#include <wiringPi.h>
#include <lcd.h>  // LCD 제어를 위한 헤더파일

int main(void)
{
    int disp1;  // LCD 디스플레이 핸들

    // GPIO 설정 실패 시 종료
    if (wiringPiSetupGpio() == -1) return 1;

    // LCD 초기화: lcdInit(rows, cols, bits, rs, strb, d4, d5, d6, d7, d0, d1, d2, d3)
    disp1 = lcdInit(2, 16, 4, 2, 4, 20, 21, 12, 16, 0, 0, 0, 0);

    lcdClear(disp1);  // LCD 화면 초기화

    // 첫 번째 줄에 "Hello World" 출력
    lcdPosition(disp1, 0, 0);  // LCD의 첫 번째 줄, 첫 번째 칸으로 커서 이동
    lcdPuts(disp1, "Hello World");  // "Hello World" 출력

    // 두 번째 줄에 "Have nice day!" 출력
    lcdPosition(disp1, 0, 1);  // LCD의 두 번째 줄, 첫 번째 칸으로 커서 이동
    lcdPuts(disp1, "Have nice day!");  // "Have nice day!" 출력

    delay(1000);  // 1초 대기

    return 0;
}