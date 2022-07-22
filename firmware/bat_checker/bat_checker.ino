#include "ble_keyboard_jis.h"
#include "driver/adc.h"
// #include <Adafruit_NeoPixel.h>

// Adafruit_NeoPixel *rgb_led; // RGB_LEDオブジェクト

// BLEキーボードクラス
BleKeyboardJIS bleKeyboard = BleKeyboardJIS();

int led_index;

void setup() {
  /*
  pinMode(19, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
  pinMode(22, INPUT_PULLUP);
  pinMode(25, INPUT_PULLUP);
  rgb_led = new Adafruit_NeoPixel(1, 27, NEO_GRB + NEO_KHZ800);
  rgb_led->setPixelColor(0, rgb_led->Color(0, 0, 0));
  rgb_led->show();
  led_index = 0;
  */
  // アナログピン初期化
  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);
  // キーボード初期化
  bleKeyboard.begin();
}

void key_press(int key_code) {
    bleKeyboard.press_raw(key_code);
    delay(50);
    bleKeyboard.release_raw(key_code);
    delay(50);
}

void loop() {
  unsigned long n = millis() / 60000; // 経過分
  int i, v;
  char s[32];
  unsigned long sn = millis(); // 開始
  v = adc1_get_voltage(ADC1_CHANNEL_0);
  if(bleKeyboard.isConnected()) {
    sprintf(s, "%d %d", (sn / 1000), v);
    i = 0;
    while (s[i]) {
      if (s[i] == 0x20) key_press(0x2c); // space
      if (s[i] == 0x30) key_press(0x27); // 0
      if (s[i] == 0x31) key_press(0x1E); // 1
      if (s[i] == 0x32) key_press(0x1F); // 2
      if (s[i] == 0x33) key_press(0x20); // 3
      if (s[i] == 0x34) key_press(0x21); // 4
      if (s[i] == 0x35) key_press(0x22); // 5
      if (s[i] == 0x36) key_press(0x23); // 6
      if (s[i] == 0x37) key_press(0x24); // 7
      if (s[i] == 0x38) key_press(0x25); // 8
      if (s[i] == 0x39) key_press(0x26); // 9
      i++;
    }
    key_press(0x28); // enter
  }
  unsigned long en = millis(); // 終了
  // 1分間待つ
  delay(60000 - (en - sn));

  /*
  if (!digitalRead(19)) {
    rgb_led->setPixelColor(0, rgb_led->Color(1, 0, 0));
  } else if (!digitalRead(21)) {
    rgb_led->setPixelColor(0, rgb_led->Color(1, 1, 0));
  } else if (!digitalRead(22)) {
    rgb_led->setPixelColor(0, rgb_led->Color(1, 1, 1));
  } else if (!digitalRead(25)) {
    rgb_led->setPixelColor(0, rgb_led->Color(1, 0, 1));
  } else {
    rgb_led->setPixelColor(0, rgb_led->Color(0, 0, 0));
  }
  rgb_led->show();
  delay(50);
  */
}
