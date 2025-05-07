#include <wiringPi.h>  // GPIO 핀 제어를 위한 헤더파일
#include <softTone.h>  // 소프트웨어 톤 생성 라이브러리 헤더파일

#define BUZZER_PIN 17   // 버저가 연결된 핀 번호 정의
#define DO_L 523        // '도' 음의 주파수(523Hz)
#define RE 587          // '레' 음의 주파수(587Hz)
#define MI 659          // '미' 음의 주파수(659Hz)
#define FA 698          // '파' 음의 주파수(698Hz)
#define SOL 784         // '솔' 음의 주파수(784Hz)
#define RA 880          // '라' 음의 주파수(880Hz)
#define SI 987          // '시' 음의 주파수(987Hz)
#define DO_H 1046       // 높은 '도' 음의 주파수(1046Hz)

// 입력된 숫자에 따라 각 음계에 해당하는 주파수를 반환하는 함수
unsigned int SevenScale (unsigned char scale)
{
    unsigned int _ret = 0;
    switch (scale)
    {
        case 0: _ret = DO_L; break;   // 0일 경우 '도' 주파수 반환
        case 1: _ret = RE; break;     // 1일 경우 '레' 주파수 반환
        case 2: _ret = MI; break;     // 2일 경우 '미' 주파수 반환
        case 3: _ret = FA; break;     // 3일 경우 '파' 주파수 반환
        case 4: _ret = SOL; break;    // 4일 경우 '솔' 주파수 반환
        case 5: _ret = RA; break;     // 5일 경우 '라' 주파수 반환
        case 6: _ret = SI; break;     // 6일 경우 '시' 주파수 반환
        case 7: _ret = DO_H; break;   // 7일 경우 높은 '도' 주파수 반환
    }
    return _ret;  // 해당 주파수 반환
}

// 버저의 주파수를 설정하는 함수
void Change_FREQ(unsigned int freq)
{
    softToneWrite(BUZZER_PIN, freq);  // BUZZER_PIN에 주파수를 설정하여 소리 출력
}

// 버저의 소리를 정지시키는 함수
void STOP_FREQ (void)
{
    softToneWrite(BUZZER_PIN, 0);  // 주파수를 0으로 설정하여 소리 정지
}

// 버저 초기화 함수
void Buzzer_Init (void)
{
    softToneCreate(BUZZER_PIN);  // BUZZER_PIN에서 소프트웨어 톤 생성
    STOP_FREQ();                 // 초기에는 소리를 정지
}

// 메인 함수
int main (void)
{
    if(wiringPiSetupGpio() == -1)   // GPIO 설정 초기화 실패 시 프로그램 종료
        return 1;

    Buzzer_Init();  // 버저 초기화
    int i;

    // 도레미파솔라시도의 음계를 차례로 재생
    for(i = 0; i < 8; i++) {
        Change_FREQ(SevenScale(i)); // 각 음계의 주파수를 설정하여 소리 재생
        delay(500);                 // 각 음을 500ms(0.5초) 동안 재생
        STOP_FREQ();                // 소리를 정지하여 음 사이에 간격을 줌
    }

    while(1)
    {
        // 프로그램이 종료되지 않도록 무한 대기
    }

    return 0;
}