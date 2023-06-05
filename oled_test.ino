#include <ESP8266TimerInterrupt.h>
#include <ESP8266_ISR_Timer.h>
#include <ESP8266_ISR_Timer.hpp>


#include <ArduinoWiFiServer.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>

#include <GParser.h>
//#include <Ticker.h>
#include <parseUtils.h>
#include <unicode.h>
#include <url.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 128 // Ширина дисплея в пикселях
#define SCREEN_HEIGHT 64 // Высота дисплея в пикселях
#define BUTTON 13 // Инициализируем вход кнопки
#define LED 14 // Инициализация светодиода
#define BUTTON_CLOCK 2 // Инициализация кнопки пуска таймера
#define RELEY_OUT 10 // Инциализация реле
#define TIMER_INTERVAL_MS 1000

boolean button_clock_state;
boolean button_clicked_state = false;
boolean button1S;   // храним состояния кнопок (S - State)
boolean button1F;   // флажки кнопок (F - Flag)
boolean button1R;   // флажки кнопок на отпускание (R - Release)
boolean button1P;   // флажки кнопок на нажатие (P - Press)
boolean button1H;   // флажки кнопок на удержание (многократный вызов) (H - Hold)
boolean button1HO;  // флажки кнопок на удержание (один вызов при нажатии) (HO - Hold Once)
boolean button1D;   // флажки кнопок на двойное нажатие (D - Double)
boolean button1DP;  // флажки кнопок на двойное нажатие и отпускание (D - Double Pressed)
boolean led_state;  //Флажек состояния светодиода
boolean led_trigger = false; // светодиод выключен
#define double_timer 150   // время (мс), отведённое на второе нажатие
#define hold 500           // время (мс), после которого кнопка считается зажатой
#define debounce 80        // (мс), антидребезг
volatile unsigned long timer_second = 0; // Переменная для отслеживания времени
unsigned long timer_stop = 0;
unsigned long button1_timer; // таймер последнего нажатия кнопки
unsigned long button_clock_timer;
unsigned long button1_double; // таймер двойного нажатия кнопки
unsigned long led_timer; // таймер маргания светодиодом
unsigned long clock_timer = 0; // Инициализация таймера часов
int led_delay = 50; // задержка на светодиод
int counter = 0;
byte byte_data;
//char str[10];  
// Инициализация объекта дисплея
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
ESP8266Timer ITimer; // Создаем обьект таймера
volatile boolean timer_expired = false;


void setup() {
  // Инициализация связи с дисплеем
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Адрес 0x3C для 128x64

  // Очистка буфера дисплея
  display.clearDisplay();

  // Установка начальных параметров дисплея
  display.setTextSize(5);            // Размер шрифта 1
  display.setTextColor(SSD1306_WHITE); // Белый цвет текста
  display.setCursor(0, 0);            // Установка позиции курсора в верхний левый угол

  // Отображение текста на дисплее
  display.println("Hello");

  // Отображение изображения на дисплее
  //display.drawBitmap(0, 16, logo, 128, 48, WHITE);
 
  // буфераОтправка  дисплея на отображение
  display.display();

  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON_CLOCK, INPUT_PULLUP);
  pinMode(RELEY_OUT, OUTPUT);

  // Interval in microsecs
  if (ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, custom_counter))
  {
    unsigned long lastMillis = millis();
    Serial.print(F("Starting  ITimer OK, millis() = ")); Serial.println(lastMillis);
  }
  else
    Serial.println(F("Can't set ITimer correctly. Select another freq. or interval"));
}
    //button1S = false;
void loop() {
    //Serial.println(timer_second);
    // -------опрос последовательного порта------
    if(Serial.available()){
      char s_tr[30];
      int a_mount = Serial.readBytesUntil(';', s_tr, 30);
      s_tr[a_mount] = NULL;

      GParser data(s_tr, ',');
      // data.getInt() -> return int; data.getFloat() -> return float; data.equals(2, "Hi") -> return boolean
      int range = data.split();
      
      switch(atoi(data[0])){
        case 0:
          if(atoi(data[1])){ // обработка флажка светодиода
            led_state = true;
          }else {
            led_state = false;
          }

          if(atoi(data[2]) != 0) { // обработка ползунка, управление частотой мигания
            int range_ = atoi(data[2]);
            map(range_, 0, 255, 0 ,50);
            constrain(range_, 0, 50);
            led_delay = range_;
          }
          
          break;
        case 1:
          break;
      }
    }
    //-------опрос кнопок--------
    button1S = !digitalRead(BUTTON);
    button_clock_state = !digitalRead(BUTTON_CLOCK);
    buttons(); //отработка кнопок
    //-------опрос кнопок--------

    if(led_state){
        led_on();
    }
    
    if (button1P) {
      counter++;
      //sprintf(str, "%d", counter);
      byte_data = byte(counter);
      Serial.println(byte_data);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println( counter );
      display.display();
      button1P = 0;
     }
     if (button1D) {
      counter = counter - 1;
       byte_data = byte(counter);
      Serial.println(byte_data);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println( counter );
      display.display();
      button1D = 0;
     }
    if (button1H && button1HO) {
       byte_data = byte(counter);
      Serial.println(byte_data);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println( counter );
      display.display();
      button1HO = 0;
    }
    if (timer_second  >= timer_stop && !timer_expired) {
      digitalWrite(RELEY_OUT, HIGH);
      timer_expired = true;
      Serial.println("The time has passed");
    }
}

void led_on() {
    if(millis() - led_timer > led_delay ) {
      digitalWrite(LED, !led_trigger);
      led_trigger = !led_trigger;
      led_timer = millis();
    }
}
//------------------------ОТРАБОТКА КНОПОК-------------------------
void buttons() {
  //-------------------------button1--------------------------
  // нажали (с антидребезгом)
  if (button1S && !button1F && millis() - button1_timer > debounce) {
    button1F = 1;
    button1HO = 1;
    button1_timer = millis();
  }
  // если отпустили до hold, считать отпущенной
  if (!button1S && button1F && !button1R && !button1DP && millis() - button1_timer < hold) {
    button1R = 1;
    button1F = 0; 
    button1_double = millis();
  }
  // если отпустили и прошло больше double_timer, считать 1 нажатием
  if (button1R && !button1DP && millis() - button1_double > double_timer) {
    button1R = 0;
    button1P = 1;
  }
  // если отпустили и прошло меньше double_timer и нажата снова, считать что нажата 2 раз
  if (button1F && !button1DP && button1R && millis() - button1_double < double_timer) {
    button1F = 0;
    button1R = 0;
    button1DP = 1;
  }
  // если была нажата 2 раз и отпущена, считать что была нажата 2 раза
  if (button1DP && millis() - button1_timer < hold) {
    button1DP = 0;
    button1D = 1;
    button1_timer = millis();
  }
  // Если удерживается более hold, то считать удержанием
  if (button1F && !button1D && !button1H && millis() - button1_timer > hold) {
    button1H = 1;
  }
  // Если отпущена после hold, то считать, что была удержана
  if (!button1S && button1F && millis() - button1_timer > hold) {
    button1F = 0;
    button1H = 0;
    button1_timer = millis();
  }
  // Обработка кнопки таймера 
  if (button_clock_state && !button_clicked_state && millis() - button_clock_timer > debounce) {
    if(!button_clicked_state){
      timer_stop = timer_second + counter;
    }
    //timer_expired = true;
    button_clicked_state = true;
    button_clock_timer = millis();
    Serial.println("Push_timer_button");
  }
  if (!button_clock_state && button_clicked_state){
    button_clicked_state = false;
  }
  //-------------------------button1--------------------------
}
//------------------------ОТРАБОТКА КНОПОК-------------------------

// -----------------Функция обработки таймера-----------------------
void custom_counter() {
   timer_second ++; 
}
