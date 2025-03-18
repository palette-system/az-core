#include "../../az_config.h"

#if KEYBOARD_TYPE == 1
// 1 = Normal BLE

#include "c6_keyboard_jis.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"

#include "HIDTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"


#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "BLEDevice";
#endif


/* ====================================================================================================================== */
/** Remap Output コールバック クラス */
/* ====================================================================================================================== */


AztoolOutputCallbacks::AztoolOutputCallbacks(void) {
	remap_change_flag = 0;
}

void AztoolOutputCallbacks::onWrite(BLECharacteristic* me) {
	int i;
	uint8_t* data = (uint8_t*)(me->getValue().c_str());
	size_t data_length = me->getLength();
	memcpy(remap_buf, data, data_length);

    // 省電力モードの場合解除
    if (hid_power_saving_mode == 1 && hid_power_saving_state == 1) { // 省電力モードON で、現在の動作モードが省電力
        hid_power_saving_state = 2;
    }

    if (remap_buf[0] == id_get_file_data) {
		// 0x31 ファイルデータ要求
		int s, p, h, l, m, j;
		// 情報を取得
		s = remap_buf[1]; // ステップ数
		p = (remap_buf[2] << 16) + (remap_buf[3] << 8) + remap_buf[4]; // 読み込み開始位置
		h = (remap_buf[5] << 24) + (remap_buf[6] << 16) + (remap_buf[7] << 8) + remap_buf[8]; // ハッシュ値
		if (h != 0) {
			l = s * (data_length - 4); // ステップ数 x 1コマンドで送るデータ数
			m = azcrc32(&save_file_data[p - l], l); // 前回送った所のハッシュを計算
			if (h != m) { // ハッシュ値が違えば前に送った所をもう一回送る
				// Serial.printf("NG : [%d %d] [ %d -> %d ]\n", h, m, p, (p - l));
				p = p - l;
			}
		}
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
			this->sendRawData(send_buf, 32);
			if (p >= save_file_length) break;

		}
		if (p >= save_file_length) {
			// Serial.printf("free load: %d %d\n", save_file_length, heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
			free(save_file_data);
		}

	} else {
		// それ以外は共通処理
		HidrawCallbackExec(data_length);
		// 返信データ送信
		if (send_buf[0]) {
			this->sendRawData(send_buf, data_length);
		}
	}

}

// Remapにデータを返す
void AztoolOutputCallbacks::sendRawData(uint8_t *data, uint8_t data_length) {
	this->pInputCharacteristic->setValue(data, data_length);
    this->pInputCharacteristic->notify();
	// delay(1);
}



BleKeyboardC6::BleKeyboardC6(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) 
    : hid(0)
    , deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0,15))
    , batteryLevel(batteryLevel) {}



void BleKeyboardC6::begin()
{
  BLEDevice::init(deviceName.c_str());
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  hid = new BLEHIDDevice(pServer);
  // キーボード 0x01
  inputKeyboard = hid->inputReport(REPORT_KEYBOARD_ID);  // <-- input REPORTID from report map
  outputKeyboard = hid->outputReport(REPORT_KEYBOARD_ID);

  // メディアキー 0x02
  inputMediaKeys = hid->inputReport(REPORT_MEDIA_KEYS_ID);
  outputKeyboard->setCallbacks(this);

  // マウス 0x03
  inputMouse = hid->inputReport(REPORT_MOUSE_ID);

  // AZTOOL 0x04
  inputAztool = hid->inputReport(REPORT_AZTOOL_ID);
  outputAztool = hid->outputReport(REPORT_AZTOOL_ID);
  aztoolCallbacks = new AztoolOutputCallbacks();
  aztoolCallbacks->pInputCharacteristic = inputAztool;
  outputAztool->setCallbacks(aztoolCallbacks);

  hid->manufacturer()->setValue(deviceManufacturer.c_str());

  hid->pnp(0x02, vid, pid, version);
  hid->hidInfo(0x00, 0x01);


  BLESecurity* pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  /*
    https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/bluedroid/ble/gatt_security_server/tutorial/Gatt_Security_Server_Example_Walkthrough.md
    ESP_LE_AUTH_NO_BOND: No bonding.
    ESP_LE_AUTH_BOND: Bonding is performed.
    ESP_LE_AUTH_REQ_MITM: MITM Protection is enabled.
    ESP_LE_AUTH_REQ_SC_ONLY: Secure Connections without bonding enabled.
    ESP_LE_AUTH_REQ_SC_BOND: Secure Connections with bonding enabled.
    ESP_LE_AUTH_REQ_SC_MITM: Secure Connections with MITM Protection and no bonding enabled.
    ESP_LE_AUTH_REQ_SC_MITM_BOND: Secure Connections with MITM Protection and bonding enabled.
    パスコード付き
    https://note.com/learninghorse/n/n6b668637f0dc
  */

  hid->reportMap((uint8_t*)_hidReportDescriptorDefault, sizeof(_hidReportDescriptorDefault));
  hid->startServices();

  onStarted(pServer);

  advertising = pServer->getAdvertising();
  // https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLEHIDDevice.h
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  // アドバタイジング間隔(Bluetooth機器を探す時の信号 指定値×0.625ミリ秒)
  // デフォルト160(100ミリ秒)らしい
  // https://reference.arduino.cc/reference/en/libraries/arduinoble/ble.setadvertisinginterval/
  // advertising->setMinInterval(0x20); // 0x20
  // advertising->setMaxInterval(0x40); // 0x40
  advertising->setScanResponse(true);
  // advertising->setScanResponse(false);
  // https://qiita.com/IRumA/items/00fc746892570f8d1c38
  // advertising->setMinPreferred(0x06); 
  // advertising->setMinPreferred(0x12);
  advertising->start();
  hid->setBatteryLevel(batteryLevel);

  // データ通信のインターバル設定
  // pServer->updateConnParams(hid_conn_handle, 100, 120, 0, 200);


  ESP_LOGD(LOG_TAG, "Advertising started!");
}

void BleKeyboardC6::end(void)
{
}

bool BleKeyboardC6::isConnected(void) {
  return this->connected;
}

void BleKeyboardC6::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0)
    this->hid->setBatteryLevel(this->batteryLevel);
}

//must be called before begin in order to set the name
void BleKeyboardC6::setName(std::string deviceName) {
  this->deviceName = deviceName;
}

/**
 * @brief Sets the waiting time (in milliseconds) between multiple keystrokes in NimBLE mode.
 * 
 * @param ms Time in milliseconds
 */
void BleKeyboardC6::setDelay(uint32_t ms) {
  this->_delay_ms = ms;
}

void BleKeyboardC6::set_vendor_id(uint16_t vid) { 
	this->vid = vid; 
}

void BleKeyboardC6::set_product_id(uint16_t pid) { 
	this->pid = pid; 
}

void BleKeyboardC6::set_version(uint16_t version) { 
	this->version = version; 
}

void BleKeyboardC6::sendReport(KeyReport* keys)
{
  if (this->isConnected())
  {
    this->inputKeyboard->setValue((uint8_t*)keys, sizeof(KeyReport));
    this->inputKeyboard->notify();
  }	
}

void BleKeyboardC6::sendReport(MediaKeyReport* keys)
{
  if (this->isConnected())
  {
    this->inputMediaKeys->setValue((uint8_t*)keys, sizeof(MediaKeyReport));
    this->inputMediaKeys->notify();
  }	
}

extern



uint8_t USBPutChar(uint8_t c);

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t BleKeyboardC6::press(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers |= (1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = pgm_read_byte(_asciimap + k);
		if (!k) {
			setWriteError();
			return 0;
		}
		if (k & 0x80) {						// it's a capital letter or other character reached with shift
			_keyReport.modifiers |= 0x02;	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
	if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
		_keyReport.keys[2] != k && _keyReport.keys[3] != k &&
		_keyReport.keys[4] != k && _keyReport.keys[5] != k) {

		for (i=0; i<6; i++) {
			if (_keyReport.keys[i] == 0x00) {
				_keyReport.keys[i] = k;
				break;
			}
		}
		if (i == 6) {
			setWriteError();
			return 0;
		}
	}
	sendReport(&_keyReport);
	return 1;
}

size_t BleKeyboardC6::press(const MediaKeyReport k)
{
    uint16_t k_16 = k[1] | (k[0] << 8);
    uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);

    mediaKeyReport_16 |= k_16;
    _mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
    _mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

	sendReport(&_mediaKeyReport);
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t BleKeyboardC6::release(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers &= ~(1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = pgm_read_byte(_asciimap + k);
		if (!k) {
			return 0;
		}
		if (k & 0x80) {							// it's a capital letter or other character reached with shift
			_keyReport.modifiers &= ~(0x02);	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (i=0; i<6; i++) {
		if (0 != k && _keyReport.keys[i] == k) {
			_keyReport.keys[i] = 0x00;
		}
	}

	sendReport(&_keyReport);
	return 1;
}

size_t BleKeyboardC6::release(const MediaKeyReport k)
{
    uint16_t k_16 = k[1] | (k[0] << 8);
    uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);
    mediaKeyReport_16 &= ~k_16;
    _mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
    _mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

	sendReport(&_mediaKeyReport);
	return 1;
}

void BleKeyboardC6::releaseAll(void)
{
	_keyReport.keys[0] = 0;
	_keyReport.keys[1] = 0;
	_keyReport.keys[2] = 0;
	_keyReport.keys[3] = 0;
	_keyReport.keys[4] = 0;
	_keyReport.keys[5] = 0;
	_keyReport.modifiers = 0;
    _mediaKeyReport[0] = 0;
    _mediaKeyReport[1] = 0;
	sendReport(&_keyReport);
	sendReport(&_mediaKeyReport);
}

size_t BleKeyboardC6::write(uint8_t c)
{
	uint8_t p = press(c);  // Keydown
	release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
}

size_t BleKeyboardC6::write(const MediaKeyReport c)
{
	uint16_t p = press(c);  // Keydown
	release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
}

size_t BleKeyboardC6::write(const uint8_t *buffer, size_t size) {
	size_t n = 0;
	while (size--) {
		if (*buffer != '\r') {
			if (write(*buffer)) {
			  n++;
			} else {
			  break;
			}
		}
		buffer++;
	}
	return n;
}

void BleKeyboardC6::onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
  this->connected = true;
  keyboard_status = 1;

  BLE2902* desc = (BLE2902*)this->inputKeyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);
  desc = (BLE2902*)this->inputMediaKeys->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);
  desc = (BLE2902*)this->inputMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);
  desc = (BLE2902*)this->inputAztool->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);

  // データ同期をとるインターバル設定
  // https://ambidata.io/samples/m5stack/m5stack_ble_sensor/
  esp_ble_conn_update_params_t conn_params = {0};
  memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
  conn_params.latency = 0;
  conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
  conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
  conn_params.timeout = 6000;     // timeout = 400*10ms = 4000ms
  //start sent the update connection parameters to the peer device.
  esp_ble_gap_update_conn_params(&conn_params);

}

void BleKeyboardC6::onDisconnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
  this->connected = false;
  keyboard_status = 0;

  BLE2902* desc = (BLE2902*)this->inputKeyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(false);
  desc = (BLE2902*)this->inputMediaKeys->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(false);
  desc = (BLE2902*)this->inputMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(false);
  desc = (BLE2902*)this->inputAztool->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(false);

  advertising->start(); // 再接続のためにもう一度Advertisingする

}

void BleKeyboardC6::onWrite(BLECharacteristic* me) {
  uint8_t* value = (uint8_t*)(me->getValue().c_str());
  (void)value;
  ESP_LOGI(LOG_TAG, "special keys: %d", *value);
}

void BleKeyboardC6::delay_ms(uint64_t ms) {
  uint64_t m = esp_timer_get_time();
  if(ms){
    uint64_t e = (m + (ms * 1000));
    if(m > e){ //overflow
        while(esp_timer_get_time() > e) { }
    }
    while(esp_timer_get_time() < e) {}
  }
}


unsigned short BleKeyboardC6::modifiers_press(unsigned short k) {
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


unsigned short BleKeyboardC6::modifiers_release(unsigned short k) {
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

unsigned short BleKeyboardC6::modifiers_media_press(unsigned short k) {
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

unsigned short BleKeyboardC6::modifiers_media_release(unsigned short k) {
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



void BleKeyboardC6::mouse_click(uint8_t b) {
    this->_MouseButtons = b;
    this->mouse_move(0,0,0,0);
    delay(10);
    this->_MouseButtons = 0x00;
    this->mouse_move(0,0,0,0);
};
void BleKeyboardC6::mouse_press(uint8_t b) {
    this->_MouseButtons |= b;
    this->mouse_move(0,0,0,0);
};
void BleKeyboardC6::mouse_release(uint8_t b) {
    this->_MouseButtons &= ~(b);
    this->mouse_move(0,0,0,0);
};
void BleKeyboardC6::mouse_move(signed char x, signed char y, signed char wheel, signed char hWheel) {
    if (this->isConnected()) {
        if (x != 0 || y != 0 || wheel != 0 || hWheel != 0 || this->_MouseButtons != 0) {
            this->setConnInterval(0); // 消費電力モード解除
        }
        uint8_t m[5];
        m[0] = this->_MouseButtons;
        m[1] = x;
        m[2] = y;
        m[3] = wheel;
        m[4] = hWheel;
        this->inputMouse->setValue(m, 5);
        this->inputMouse->notify();
    }
};
size_t BleKeyboardC6::press_set(uint8_t k) {
    unsigned short kk;
    kk = _asciimap[k];
    if (!kk) return 0;
    this->_keyReport.modifiers = 0x00;
    this->_keyReport.keys[0] = this->modifiers_press(kk);
    this->_keyReport.keys[1] = 0x00;
    this->_keyReport.keys[2] = 0x00;
    this->_keyReport.keys[3] = 0x00;
    this->_keyReport.keys[4] = 0x00;
    this->_keyReport.keys[5] = 0x00;

    this->sendReport(&_keyReport);
    return 1;
}; // 指定したキーだけ押す
size_t BleKeyboardC6::press_raw(unsigned short k) {
  // Serial.printf("press_raw %d\r\n", k);
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
size_t BleKeyboardC6::release_raw(unsigned short k) {
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
void BleKeyboardC6::setConnInterval(int interval_type) {};

#endif // KEYBOARD_TYPE == 1 // Normal BLE
