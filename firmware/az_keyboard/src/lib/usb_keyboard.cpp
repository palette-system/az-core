
#include "usb_keyboard.h"

USBHID HID;
bool usbhid_connected = false;

int step_count = 0; // HIDRAWファイル送信用ステップ数
int start_point = 0; // HIDRAWファイル送信用ファイル送信ポイント
int raw_len = 0; // HIDRAwファイル送信用バッファサイズ
int raw_file_flag = 0; // はいたフラグ

TaskHandle_t hidraw_send_task;

static void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    Serial.printf("HID: eventBase %d  eventID %d \r\n", event_base, event_id);
  if(event_base == ARDUINO_USB_EVENTS){
    arduino_usb_event_data_t * data = (arduino_usb_event_data_t*)event_data;
    Serial.printf("ARDUINO_USB_EVENTS: eventBase %d \r\n", ARDUINO_USB_EVENTS);
    switch (event_id){
      // 0 USB 刺された
      case ARDUINO_USB_STARTED_EVENT:
        Serial.println("USB PLUGGED");
        usbhid_connected = true;
        break;
      // 1 USB 外した
      case ARDUINO_USB_STOPPED_EVENT:
        Serial.println("USB UNPLUGGED");
        usbhid_connected = false;
        break;
      case ARDUINO_USB_SUSPEND_EVENT:
        Serial.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en);
        break;
      case ARDUINO_USB_RESUME_EVENT:
        Serial.println("USB RESUMED");
        break;
      
      default:
        break;
    }
  }
};


CustomHIDDevice::CustomHIDDevice(void){
    HID.addDevice(this, sizeof(_hidReportDescriptorDefault));
};

void CustomHIDDevice::begin(std::string deviceName, std::string deviceManufacturer){
    HID.begin();
    USB.VID(hid_vid);
    USB.PID(hid_pid);
    USB.productName(deviceName.c_str());
    USB.onEvent(usbEventCallback);
    USB.begin();
};

// HIDからreport_mapの要求
uint16_t CustomHIDDevice::_onGetDescriptor(uint8_t* buffer){
    memcpy(buffer, _hidReportDescriptorDefault, sizeof(_hidReportDescriptorDefault));
    return sizeof(_hidReportDescriptorDefault);
};

// HIDからデータを受け取る
void CustomHIDDevice::_onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len) {
    int i;
    // Serial.printf("_onOutput: report_id %d / len %d\r\n", report_id, len);
    if (report_id == REPORT_KEYBOARD_ID) { // caps lockとか
        Serial.printf("Keyboard status: %x\r\n", buffer[0]);

    } else if (report_id == INPUT_REP_REF_RAW_ID) { // HID Raw
        /*
        Serial.printf("get: ");
        for (i=0; i<len; i++) Serial.printf("%02x", buffer[i]);
        Serial.printf("\r\n");
        */
        memcpy(remap_buf, buffer, len);
        if (remap_buf[0] == id_get_file_data) {
          // 0x31 ファイルデータ要求
          int s, p, h, l, m, j;
          // 情報を取得
          step_count = remap_buf[1]; // ステップ数
          p = (remap_buf[2] << 16) + (remap_buf[3] << 8) + remap_buf[4]; // 読み込み開始位置
          h = (remap_buf[5] << 24) + (remap_buf[6] << 16) + (remap_buf[7] << 8) + remap_buf[8]; // ハッシュ値
          if (h != 0) {
            l = s * (len - 4); // ステップ数 x 1コマンドで送るデータ数
            m = azcrc32(&save_file_data[p - l], l); // 前回送った所のハッシュを計算
            if (h != m) { // ハッシュ値が違えば前に送った所をもう一回送る
              p = p - l;
            }
          }
          start_point = p;
          raw_len = len;
          while (raw_file_flag == 1) delay(10); // 前の送信処理実行中なら待つ
          xTaskCreatePinnedToCore(raw_file_data, "data_send", 2048, NULL, 5, &hidraw_send_task, 1); // 非同期でデータ送信
        } else {
          // それ以外は共通処理
          HidrawCallbackExec(len);
          // 返信データ送信
          if (send_buf[0]) {
            HID.SendReport(INPUT_REP_REF_RAW_ID, send_buf, len);
            /*
            Serial.printf("put: ");
            for (i=0; i<len; i++) Serial.printf("%02x", send_buf[i]);
            Serial.printf("\r\n");
            */
          }
        }
    }
};

// データを送信
static void raw_file_data(void* arg) {
  raw_file_flag = 1;
    int i, j, p, s;
    bool r;
    s = step_count; // ステップ数
    p = start_point; // 読み込み開始位置
    j = 0;
    for (j=0; j<s; j++) {
      send_buf[0] = id_get_file_data;
      send_buf[1] = ((p >> 16) & 0xff);
      send_buf[2] = ((p >> 8) & 0xff);
      send_buf[3] = (p & 0xff);
      i = 4;
      while (p < save_file_length) {
        send_buf[i] = save_file_data[p];
        i++;
        p++;
        if (i >= 32) break;
      }
      while (i<32) {
        send_buf[i] = 0x00;
        i++;
      }
      while (!HID.ready()) delay(10);
      r = HID.SendReport(INPUT_REP_REF_RAW_ID, send_buf, raw_len);
      if (p >= save_file_length) break;

    }
    if (p >= save_file_length) {
      free(save_file_data);
    }
  raw_file_flag = 2;
  // vTaskDelay(portMAX_DELAY); //delay(portMAX_DELAY);
  vTaskDelete(hidraw_send_task); // タスク削除
};

bool CustomHIDDevice::send(uint8_t * value){
    return HID.SendReport(REPORT_KEYBOARD_ID, value, 8);
};

bool CustomHIDDevice::mouse_send(uint8_t x) {
    uint8_t m[5];
    m[0] = 0x00;
    m[1] = x;
    m[2] = 0x00;
    m[3] = 0x00;
    m[4] = 0x00;
    return HID.SendReport(REPORT_MOUSE_ID, m, 5);
};

void CustomHIDDevice::sendReport(KeyReport* keys)
{
  if (this->isConnected()) {
    HID.SendReport(REPORT_KEYBOARD_ID, (uint8_t*)keys, sizeof(KeyReport));
  }
};

void CustomHIDDevice::sendReport(MediaKeyReport* keys)
{
  if (this->isConnected()) {
    HID.SendReport(REPORT_MEDIA_KEYS_ID, (uint8_t*)keys, sizeof(MediaKeyReport));
  }
};


unsigned short CustomHIDDevice::modifiers_press(unsigned short k) {
  if (k & JIS_SHIFT) { // shift
    this->_keyReport.modifiers |= 0x02; // the left shift modifier
    k &= 0xFF;
  }
  if (k == 224) { k = 0; this->_keyReport.modifiers |= 0x01; } // LEFT Ctrl
  if (k == 228) { k = 0; this->_keyReport.modifiers |= 0x10; } // RIGHT Ctrl
  if (k == 225) { k = 0; this->_keyReport.modifiers |= 0x02; } // LEFT Shift
  if (k == 229) { k = 0; this->_keyReport.modifiers |= 0x20; } // RIGHT Shift
  if (k == 226) { k = 0; this->_keyReport.modifiers |= 0x04; } // LEFT Alt
  if (k == 230) { k = 0; this->_keyReport.modifiers |= 0x40; } // RIGHT Alt
  if (k == 227) { k = 0; this->_keyReport.modifiers |= 0x08; } // LEFT GUI
  if (k == 231) { k = 0; this->_keyReport.modifiers |= 0x80; } // RIGHT GUI
  return k;
};


unsigned short CustomHIDDevice::modifiers_release(unsigned short k) {
  if (k & JIS_SHIFT) { // shift
    this->_keyReport.modifiers &= ~(0x02);  // the left shift modifier
    k &= 0xFF;
  }
  if (k == 224) { k = 0; this->_keyReport.modifiers &= ~(0x01); } // LEFT Ctrl
  if (k == 228) { k = 0; this->_keyReport.modifiers &= ~(0x10); } // RIGHT Ctrl
  if (k == 225) { k = 0; this->_keyReport.modifiers &= ~(0x02); } // LEFT Shift
  if (k == 229) { k = 0; this->_keyReport.modifiers &= ~(0x20); } // RIGHT Shift
  if (k == 226) { k = 0; this->_keyReport.modifiers &= ~(0x04); } // LEFT Alt
  if (k == 230) { k = 0; this->_keyReport.modifiers &= ~(0x40); } // RIGHT Alt
  if (k == 227) { k = 0; this->_keyReport.modifiers &= ~(0x08); } // LEFT GUI
  if (k == 231) { k = 0; this->_keyReport.modifiers &= ~(0x80); } // RIGHT GUI
  return k;
};

unsigned short CustomHIDDevice::modifiers_media_press(unsigned short k) {
  if (k == 8193) { // Eject
    this->_mediaKeyReport[0] |= 0x01;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8194) { // Media Next
    this->_mediaKeyReport[0] |= 0x02;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8195) { // Media Previous
    this->_mediaKeyReport[0] |= 0x04;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8196) { // Media Stop
    this->_mediaKeyReport[0] |= 0x08;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8197) { // Media play / pause
    this->_mediaKeyReport[0] |= 0x10;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8198) { // Media Mute
    this->_mediaKeyReport[0] |= 0x20;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8199) { // Media volume +
    this->_mediaKeyReport[0] |= 0x40;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8200) { // Media volume -
    this->_mediaKeyReport[0] |= 0x80;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  }
  return 0;
};

unsigned short CustomHIDDevice::modifiers_media_release(unsigned short k) {
  if (k == 8193) { // Eject
    this->_mediaKeyReport[0] &= ~(0x01);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8194) { // Media Next
    this->_mediaKeyReport[0] &= ~(0x02);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8195) { // Media Previous
    this->_mediaKeyReport[0] &= ~(0x04);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8196) { // Media Stop
    this->_mediaKeyReport[0] &= ~(0x08);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8197) { // Media play / pause
    this->_mediaKeyReport[0] &= ~(0x10);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8198) { // Media Mute
    this->_mediaKeyReport[0] &= ~(0x20);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8199) { // Media volume +
    this->_mediaKeyReport[0] &= ~(0x40);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8200) { // Media volume -
    this->_mediaKeyReport[0] &= ~(0x80);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  }
  return 0;
};


bool CustomHIDDevice::isConnected(void) {
    return usbhid_connected;
};
void CustomHIDDevice::mouse_move(signed char x, signed char y, signed char wheel, signed char hWheel) {
    Serial.printf("mouse_move %d %d %d %d\r\n", x, y, wheel, hWheel);
    if (this->isConnected()) {
        uint8_t m[5];
        m[0] = this->_MouseButtons;
        m[1] = x;
        m[2] = y;
        m[3] = wheel;
        m[4] = hWheel;
        HID.SendReport(REPORT_MOUSE_ID, m, 5);
    }
};
void CustomHIDDevice::mouse_press(uint8_t b) {
    Serial.printf("mouse_press %d\r\n", b);
    this->_MouseButtons |= b;
    this->mouse_move(0,0,0,0);
};
void CustomHIDDevice::mouse_release(uint8_t b) {
    Serial.printf("mouse_release %d\r\n", b);
    this->_MouseButtons &= ~(b);
    this->mouse_move(0,0,0,0);
};
// 指定したキーだけ押す
size_t CustomHIDDevice::press_set(uint8_t k) {
    Serial.printf("press_set %d\r\n", k);
    return 0;
};
size_t CustomHIDDevice::press_raw(unsigned short k) {
  Serial.printf("press_raw %d\r\n", k);
  uint8_t i;
  unsigned short kk;
  // メディアキー
  if (modifiers_media_press(k)) return 1;
  kk = this->modifiers_press(k);
  if (this->_keyReport.keys[0] != kk && this->_keyReport.keys[1] != kk &&
    this->_keyReport.keys[2] != kk && this->_keyReport.keys[3] != kk &&
    this->_keyReport.keys[4] != kk && this->_keyReport.keys[5] != kk) {

    for (i=0; i<6; i++) {
      if (this->_keyReport.keys[i] == 0x00) {
        this->_keyReport.keys[i] = kk;
        break;
      }
    }
    if (i == 6) {
      return 0;
    }
  }
  this->sendReport(&_keyReport);
  return 1;
};
size_t CustomHIDDevice::release_raw(unsigned short k) {
    Serial.printf("release_raw %d\r\n", k);
  uint8_t i;
  unsigned short kk;
  // メディアキー
  if (modifiers_media_release(k)) return 1;
  kk = this->modifiers_release(k);

  // Test the key report to see if k is present.  Clear it if it exists.
  // Check all positions in case the key is present more than once (which it shouldn't be)
  for (i=0; i<6; i++) {
    if (0 != kk && this->_keyReport.keys[i] == kk) {
      this->_keyReport.keys[i] = 0x00;
    }
  }

  this->sendReport(&this->_keyReport);
  return 1;
};
void CustomHIDDevice::releaseAll(void) {
    Serial.printf("releaseAll\r\n");
  this->_keyReport.keys[0] = 0;
  this->_keyReport.keys[1] = 0;
  this->_keyReport.keys[2] = 0;
  this->_keyReport.keys[3] = 0;
  this->_keyReport.keys[4] = 0;
  this->_keyReport.keys[5] = 0;
  this->_keyReport.modifiers = 0;
  this->_mediaKeyReport[0] = 0;
  this->_mediaKeyReport[1] = 0;
  this->sendReport(&this->_keyReport);
  this->sendReport(&this->_mediaKeyReport);
};
// 省電力モード設定
void CustomHIDDevice::setConnInterval(int interval_type) {
    Serial.printf("setConnInterval %d\r\n", interval_type);
};



