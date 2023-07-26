# ESP32 S3 ビルド環境

## Arduino IDE Version
 Version : 2.1.1<br>
<br><br>

## ボードマネージャー
 <b>esp32</b> by Espressif<br>
 Version : 2.0.11<br>
<br><br>
 ボード : ESP32S3 Dev Module<br>
 CPU Frequency : 80MHz<br>
 Partition Scheme : No OTA (2MB APP/2MB SPIFFS)<br>
<br><br>

![ボード設定ESP32S3](/docs/img/board_esp32s3.png)
<br><br>

## ライブラリ
<br>
 <b>ArduinoJson</b> by Benoit<br>
 Version : 6.21.3<br>
<br><br>

 <b>Adafruit NeoPixel</b> by Adafruit<br>
 Version : 1.11.0<br>
<br><br>

 <b>Adafruit MCP23017 Arduino Library</b> by Adafruit<br>
 Version : 2.3.0<br>
<br><br>

 <b>NimBLE-Arduino</b> by h2zero<br>
 Version : 1.4.1<br>
<br><br>

## ソースの場所

<a href="/firmware/az_keyboard/">ソース</a><br>
<br><br>

## ESP32 S3 用に修正する箇所

 修正ファイル : <a href="/firmware/az_keyboard/az_config.h">az_config.h</a><br>
 修正内容 : <br>
```
#define KEYBOARD_TYPE 0
#define CPUTYPE_ESP32 0

  ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓

#define KEYBOARD_TYPE 1
#define CPUTYPE_ESP32 9
```
