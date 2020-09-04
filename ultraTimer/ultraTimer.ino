#define trigPin 13 
#define echoPin 12 
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

const int led=3;
const int speakerPin=7;
const int resetKey=8; //초기화 버튼
const int stKey=9;  //시작, 정지 버튼
const int secKey=10;  //초 설정 버튼
const int minKey=11;  //분 설정 버튼

int setMinute = 0;
int setSecond = 0;  
int setTime = 0;
int curTime = 0;
unsigned long prev_time = 0;  //이전 시간 저장을 위한 변수

bool resetPressed = false;
bool stPressed = false;
bool minKeyPressed = false;
bool secKeyPressed = false;

//타이머 단계 설정
const int SETUP = 0;
const int RUNNING = 1;
const int END = 2;
const int VIEW = 3; //타이머 작동 중 멈췄을 때

int curState = SETUP; //현재 단계

LiquidCrystal_I2C lcd(0x3f, 16, 2);

void setup() 
{ 
    pinMode(speakerPin,OUTPUT);
    pinMode(led,OUTPUT);
    pinMode(minKey,INPUT);
    pinMode(secKey,INPUT);
    pinMode(stKey,INPUT);
    pinMode(resetKey, INPUT);
    pinMode(trigPin, OUTPUT); 
    pinMode(echoPin, INPUT); 
    lcd.begin();
    lcd.backlight(); 
    Serial.begin(9600);
} 

long microsecondsToInches(long microseconds)
{
   return (microseconds / 74 / 2);
}
//초음파 센서로 거리값을 읽어 반환
int ultraSensor()
{
    int duration;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2); 
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10); 
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH); 
    return microsecondsToInches(duration);
}
//타이머 시간 및 단계 리셋
void reset(){
  curState = SETUP;
  setMinute = 0;
  setSecond = 0;
}

void loop() 
{
    int val; //타이머 시간 설정을 위한 변수
    unsigned long current_time; //현재까지 지나간 시간
    
    resetPressed = false; //reset버튼
    minKeyPressed = false;  //minute버튼
    secKeyPressed = false;  //second버튼
    stPressed = false;  //start/stop 버튼
    
    if(digitalRead(resetKey) == HIGH) resetPressed = true;  
    if(digitalRead(stKey) == HIGH)  stPressed = true;
    if(digitalRead(minKey) == HIGH) minKeyPressed = true;
    if(digitalRead(secKey) == HIGH) secKeyPressed = true;

    //타이머 각 단계별 진행 과정
    switch(curState)
    {
      case SETUP:  //시간 설정
        val = ultraSensor();  //시간 설정을 위한 초음파센서 거리값 반환
        if(val>=60) val = 0;  //거리 60인치이상은 시간을 0으로 인식하기 위해
        if(resetPressed)  reset();
        if(minKeyPressed) setMinute = val;
        if(secKeyPressed) setSecond = val;
        if(stPressed)
        {
          curState = RUNNING;
          setTime = (setMinute*60)+setSecond;
          curTime = setTime;
          prev_time = millis(); //시작 시 아두이노의 시간 정보를 저장하기 시작
        }
        break;
      
      case RUNNING: //타이머 작동 중
        
        if(stPressed)  curState = VIEW; //작동 중 정지를 눌렀을 경우 그대로 멈춘다
        if(resetPressed)  curState = SETUP;
        break;

      //시간이 종료된 후 reset버튼을 누르면 부저가 멈추고 LED OFF
      case END:
        if(resetPressed) {
          curState = SETUP;
          digitalWrite(led, LOW);
          noTone(speakerPin);
          delay(100);
        }
        break;
     //타이머 작동 중 start/stop 버튼을 눌렀을 때
      case VIEW:
        
        if(stPressed) curState = RUNNING;
        if(resetPressed) curState = SETUP;
        break;
    }
    
    //LCD에 표현하는 부분
    switch(curState)
    {
      case SETUP:
        if(val>=60) val=0;
        lcd.setCursor(0,0);
        lcd.print("Time Set:");
        lcd.print(val);
        lcd.print("         ");
        lcd.setCursor(0,1);
        lcd.print(setMinute);
        lcd.print(":");
        lcd.print(setSecond);
        lcd.print("         ");
        break;
      case RUNNING:
        lcd.setCursor(0,0);
        lcd.print("Timer Running..");
        lcd.setCursor(0,1);
        if((curTime/60)<10) lcd.print("0");
        lcd.print(curTime/60);
        lcd.print(":");
        if((curTime%60)<10) lcd.print("0");
        lcd.print(curTime%60);
        break;
      case END:
        lcd.setCursor(0,0);
        lcd.print("   Timer END!!  ");
        lcd.setCursor(0,1);
        lcd.print("                ");
        break;
      case VIEW:
        lcd.setCursor(0,0);
        lcd.print("Timer Stop..   ");
        lcd.setCursor(0,1);
        if((curTime/60)<10) lcd.print("0");
        lcd.print(curTime/60);
        lcd.print(":");
        if((curTime%60)<10) lcd.print("0");
        lcd.print(curTime%60);
        break;
    }

    //시간이 흐르는 부분 관리
    switch(curState)
    {
      case SETUP:
        break;
      case RUNNING:
        current_time = millis();  //현재까지 지나온 시간
        //이전 시간과 현재 시간을 빼서 1000ms(1초)이상이면 curTime에서 1초 감소
        if(current_time - prev_time >= 1000){
          curTime--;
          prev_time = current_time; //다음 간격을 위해 현재 시간을 이전 시간에 저장
        }
        if(curTime <= 0)  curState = END;  //시간이 0보다 작아지면 끝
        break;
      case END:
        digitalWrite(led, HIGH);
        tone(speakerPin, 440);
        delay(20);
        noTone(speakerPin);
        delay(40);
        break;
    }
    delay(100);
}

