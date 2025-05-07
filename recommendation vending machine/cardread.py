from mfrc522 import SimpleMFRC522
import RPi.GPIO as GPIO
GPIO.setwarnings(False)  # GPIO 경고 메시지 비활성화

def write_id_to_file():
    reader = SimpleMFRC522()
    print("카드를 태그해주세요.")
    id, text = reader.read()  # RFID ID 읽기
    with open("rfid_data.txt", "w") as file:  # 파일에 ID 쓰기
        file.write(str(id))

if __name__ == "__main__":
    write_id_to_file()
