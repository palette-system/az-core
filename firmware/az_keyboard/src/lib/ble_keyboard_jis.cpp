#include "../../az_config.h"

#if KEYBOARD_TYPE == 0
// 0 = Nim BLE
// 0x00 = ノーマルESP32

#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEServer.h>
#include "HIDTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"

#include "ble_keyboard_jis.h"
#include "ble_callbacks.h"

// コールバッククラス
static DescriptorCallbacks dscCallbacks;
static CharacteristicCallbacks chrCallbacks;
static RemapDescriptorCallbacks RemapDscCallbacks;


/* ====================================================================================================================== */
/** Characteristic コールバック クラス */
/* ====================================================================================================================== */

CharacteristicCallbacks::CharacteristicCallbacks(void) {
};

void CharacteristicCallbacks::onRead(NimBLECharacteristic* pCharacteristic){
    // Serial.print(pCharacteristic->getUUID().toString().c_str());
    // Serial.print(": onRead(), value: ");
    // Serial.println(pCharacteristic->getValue().c_str());
};

void CharacteristicCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    // Serial.print(pCharacteristic->getUUID().toString().c_str());
    // Serial.print(": onWrite(), value: ");
    // Serial.println(pCharacteristic->getValue().c_str());
};

void CharacteristicCallbacks::onNotify(NimBLECharacteristic* pCharacteristic) {
    // Serial.println("Sending notification to clients");
};

void CharacteristicCallbacks::onStatus(NimBLECharacteristic* pCharacteristic, int code) {
    String str = ("Notf/Ind stscode: ");
    str += "retcode: ";
    str += code;
    str += ", "; 
    str += NimBLEUtils::returnCodeToString(code);
    //Serial.print(str);
};

void CharacteristicCallbacks::onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
    String str = "Client ID: ";
    str += desc->conn_handle;
    str += " Address: ";
    str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();
    if(subValue == 0) {
        str += " Unsubscribed to ";
    }else if(subValue == 1) {
        str += " Subscribed to notfications for ";
    } else if(subValue == 2) {
        str += " Subscribed to indications for ";
    } else if(subValue == 3) {
        str += " Subscribed to notifications and indications for ";
    }
    str += std::string(pCharacteristic->getUUID()).c_str();

    // Serial.println(str);
};

/* ====================================================================================================================== */
/** Descriptor コールバック クラス */
/* ====================================================================================================================== */

DescriptorCallbacks::DescriptorCallbacks(void) {
};

void DescriptorCallbacks::onWrite(NimBLEDescriptor* pDescriptor) {
    // std::string dscVal((char*)pDescriptor->getValue(), pDescriptor->getLength());
    // Serial.print("Descriptor witten value:");
    // Serial.println(dscVal.c_str());
};

void DescriptorCallbacks::onRead(NimBLEDescriptor* pDescriptor) {
    // Serial.print(pDescriptor->getUUID().toString().c_str());
    // Serial.println(" Descriptor read");
};

/* ====================================================================================================================== */
/** コネクションクラス */
/* ====================================================================================================================== */

BleConnectionStatus::BleConnectionStatus(void) {
};

void BleConnectionStatus::onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc)
{
  keyboard_status = 1;
  this->connected = true;
  hid_conn_handle = desc->conn_handle;
  if (hid_power_saving_mode == 1) {
	pServer->updateConnParams(desc->conn_handle, hid_interval_normal - 2, hid_interval_normal + 2, 0, 60);
    hid_power_saving_state = 0; // 通常モード
    hid_state_change_time = millis();
  }
};

void BleConnectionStatus::onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc)
{
  keyboard_status = 0;
  this->connected = false;
  hid_conn_handle = 0;
};

/* ====================================================================================================================== */
/** Output コールバック クラス */
/* ====================================================================================================================== */

KeyboardOutputCallbacks::KeyboardOutputCallbacks(void) {
}

void KeyboardOutputCallbacks::onWrite(NimBLECharacteristic* me) {
  uint8_t* value = (uint8_t*)(me->getValue().c_str());
  ESP_LOGI(LOG_TAG, "special keys: %d", *value);
}


/* ====================================================================================================================== */
/** Remap Descriptor コールバック クラス */
/* ====================================================================================================================== */

RemapDescriptorCallbacks::RemapDescriptorCallbacks(void) {
};

void RemapDescriptorCallbacks::onWrite(NimBLEDescriptor* pDescriptor) {
    // std::string dscVal((char*)pDescriptor->getValue(), pDescriptor->getLength());
    // Serial.print("RemapDescriptorCallbacks: onWrite: ");
    // Serial.println(dscVal.c_str());
};

void RemapDescriptorCallbacks::onRead(NimBLEDescriptor* pDescriptor) {
    // Serial.print("RemapDescriptorCallbacks: onRead: ");
    // Serial.println(pDescriptor->getUUID().toString().c_str());
};


/* ====================================================================================================================== */
/** Remap Output コールバック クラス */
/* ====================================================================================================================== */


RemapOutputCallbacks::RemapOutputCallbacks(void) {
	remap_change_flag = 0;
}


void RemapOutputCallbacks::onWrite(NimBLECharacteristic* me) {
	int i;
	uint8_t* data = (uint8_t*)(me->getValue().c_str());
	size_t data_length = me->getLength();
	memcpy(remap_buf, data, data_length);

    // 省電力モードの場合解除
    if (hid_power_saving_mode == 1 && hid_power_saving_state == 1) { // 省電力モードON で、現在の動作モードが省電力
        hid_power_saving_state = 2;
    }

    /* REMAP から受け取ったデータデバッグ表示
	Serial.printf("get: (%d) ", data_length);
	for (i=0; i<data_length; i++) {
		Serial.printf("%02x ", remap_buf[i]);
	}
	Serial.printf("\n");
	*/
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


    /*
	Serial.printf("put: (%d) ", data_length);
	for (i=0; i<data_length; i++) Serial.printf("%02x ", send_buf[i]);
	Serial.printf("\n\n");
	*/

	
}

// Remapにデータを返す
void RemapOutputCallbacks::sendRawData(uint8_t *data, uint8_t data_length) {
	this->pInputCharacteristic->setValue(data, data_length);
    this->pInputCharacteristic->notify();
	// delay(1);
}



// コンストラクタ
BleKeyboardJIS::BleKeyboardJIS(void)
{
  this->_MouseButtons = 0x00;
  this->batteryLevel = 100;
  this->connectionStatus = new BleConnectionStatus();
  this->_hidReportDescriptor = (uint8_t *)_hidReportDescriptorDefault;
  this->_hidReportDescriptorSize = sizeof(_hidReportDescriptorDefault);
  
};


// HID report map 設定
// 例： bleKeyboard.set_report_map((uint8_t *)_hidReportDescriptorDefault, sizeof(_hidReportDescriptorDefault));
void BleKeyboardJIS::set_report_map(uint8_t * report_map, unsigned short report_size)
{
  this->_hidReportDescriptor = report_map;
  this->_hidReportDescriptorSize = report_size;
};

// BLEキーボードとして処理開始
void BleKeyboardJIS::begin(std::string deviceName, std::string deviceManufacturer)
{
  this->deviceName = deviceName;
  this->deviceManufacturer = deviceManufacturer;
  xTaskCreate(this->taskServer, "server", 20000, (void *)this, 5, NULL); // BLE HID 開始処理
};

// 接続中かどうかを返す
bool BleKeyboardJIS::isConnected(void)
{
  return (this->connectionStatus->connected && keyboard_status == 1);
};

// BLE HID 開始処理
void BleKeyboardJIS::taskServer(void* pvParameter)
{

   BleKeyboardJIS *bleKeyboardInstance = (BleKeyboardJIS *) pvParameter; //static_cast<BleKeyboard *>(pvParameter);

    /** sets device name */
    NimBLEDevice::init(bleKeyboardInstance->deviceName);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);

    bleKeyboardInstance->pServer = NimBLEDevice::createServer();
    bleKeyboardInstance->pServer->setCallbacks(bleKeyboardInstance->connectionStatus);

    //DeviceInfoService
    bleKeyboardInstance->pDeviceInfoService = bleKeyboardInstance->pServer->createService("180A"); // <-デバイスインフォのUUID
    bleKeyboardInstance->pPnpCharacteristic = bleKeyboardInstance->pDeviceInfoService->createCharacteristic("2A50", NIMBLE_PROPERTY::READ);
    uint8_t sig = 0x02;
    uint16_t version = 0x0210; 
    uint8_t pnp[] = { sig, (uint8_t) (hid_vid >> 8), (uint8_t) hid_vid, (uint8_t) (hid_pid >> 8), (uint8_t) hid_pid, (uint8_t) (version >> 8), (uint8_t) version };
    bleKeyboardInstance->pPnpCharacteristic->setValue(pnp, sizeof(pnp));
    bleKeyboardInstance->pPnpCharacteristic->setCallbacks(&chrCallbacks);

    //DeviceInfoService-Manufacturer
    bleKeyboardInstance->pManufacturerCharacteristic = bleKeyboardInstance->pDeviceInfoService->createCharacteristic("2A29", NIMBLE_PROPERTY::READ); // 0x2a29 = メーカー名
    bleKeyboardInstance->pManufacturerCharacteristic->setValue(bleKeyboardInstance->deviceManufacturer);
    bleKeyboardInstance->pManufacturerCharacteristic->setCallbacks(&chrCallbacks);

    //HidService
    bleKeyboardInstance->pHidService = bleKeyboardInstance->pServer->createService(NimBLEUUID("1812"));

    //HidService-hidInfo
    bleKeyboardInstance->pHidInfoCharacteristic = bleKeyboardInstance->pHidService->createCharacteristic("2A4A", NIMBLE_PROPERTY::READ);// HID Information 会社名？とか？あと何かのフラグ
    uint8_t country = 0x00;
    uint8_t flags = 0x01;
    uint8_t info[] = { 0x11, 0x1, country, flags };
    bleKeyboardInstance->pHidInfoCharacteristic->setValue(info, sizeof(info));
    bleKeyboardInstance->pHidInfoCharacteristic->setCallbacks(&chrCallbacks);

    //HidService-reportMap
    ESP_LOGD(LOG_TAG, "reportMap set: start");
    ESP_LOGD(LOG_TAG, "reportMap set: size %D", bleKeyboardInstance->_hidReportDescriptorSize);
    bleKeyboardInstance->pReportMapCharacteristic = bleKeyboardInstance->pHidService->createCharacteristic("2A4B",NIMBLE_PROPERTY::READ); // HID Report Map (ここでHIDのいろいろ設定
    bleKeyboardInstance->pReportMapCharacteristic->setValue((uint8_t*)bleKeyboardInstance->_hidReportDescriptor, bleKeyboardInstance->_hidReportDescriptorSize);
    bleKeyboardInstance->pReportMapCharacteristic->setCallbacks(&chrCallbacks);

    //HidService-HidControl
    bleKeyboardInstance->pHidControlCharacteristic = bleKeyboardInstance->pHidService->createCharacteristic("2A4C", NIMBLE_PROPERTY::WRITE_NR);// HID Control Point
    bleKeyboardInstance->pHidControlCharacteristic->setCallbacks(&chrCallbacks);

    //HidService-protocolMode
    bleKeyboardInstance->pProtocolModeCharacteristic = bleKeyboardInstance->pHidService->createCharacteristic("2A4E",NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ); // Protocol Mode
    const uint8_t pMode[] = { 0x01 }; // 0: Boot Protocol 1: Rport Protocol
    bleKeyboardInstance->pProtocolModeCharacteristic->setValue((uint8_t*) pMode, 1);
    bleKeyboardInstance->pProtocolModeCharacteristic->setCallbacks(&chrCallbacks);

    //HidService-input
    bleKeyboardInstance->pInputCharacteristic = bleKeyboardInstance->pHidService->createCharacteristic("2A4D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC); // Report
    bleKeyboardInstance->pDesc1 = bleKeyboardInstance->pInputCharacteristic->createDescriptor( "2908", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC, 20); // Report Reference
    uint8_t desc1_val[] = { 0x01, 0x01 }; // Report ID 1 を Input に設定
    bleKeyboardInstance->pDesc1->setValue((uint8_t*) desc1_val, 2);
    bleKeyboardInstance->pDesc1->setCallbacks(&dscCallbacks);


    // HidService-output
    bleKeyboardInstance->pOutputCharacteristic = bleKeyboardInstance->pHidService->createCharacteristic("2A4D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC);
    bleKeyboardInstance->pOutputCharacteristic->setCallbacks(new KeyboardOutputCallbacks());
    bleKeyboardInstance->pDesc2 = bleKeyboardInstance->pOutputCharacteristic->createDescriptor("2908", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC, 20);
    uint8_t desc1_val2[] = { 0x01, 0x02}; // Report ID 1 を Output に設定
    bleKeyboardInstance->pDesc2->setValue((uint8_t*) desc1_val2, 2);


    //HidService-input2 media port
    bleKeyboardInstance->pInputCharacteristic2 = bleKeyboardInstance->pHidService->createCharacteristic("2A4D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC); // Report
    bleKeyboardInstance->pDesc3 = bleKeyboardInstance->pInputCharacteristic2->createDescriptor("2908", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC, 20);
    uint8_t desc1_val3[] = { 0x02, 0x01 }; // Report ID 2 を Input に設定
    bleKeyboardInstance->pDesc3->setValue((uint8_t*) desc1_val3, 2);
    bleKeyboardInstance->pDesc3->setCallbacks(&dscCallbacks);


    //HidService-input3 media port
    bleKeyboardInstance->pInputCharacteristic3 = bleKeyboardInstance->pHidService->createCharacteristic("2A4D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC); // Report
    bleKeyboardInstance->pDesc4 = bleKeyboardInstance->pInputCharacteristic3->createDescriptor("2908", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC, 20);
    uint8_t desc1_val4[] = { 0x03, 0x01 }; // Report ID 3 を Input に設定
    bleKeyboardInstance->pDesc4->setValue((uint8_t*) desc1_val4, 2);
    bleKeyboardInstance->pDesc4->setCallbacks(&dscCallbacks);

    
    //BatteryService
    bleKeyboardInstance->pBatteryService = bleKeyboardInstance->pServer->createService("180F");
    bleKeyboardInstance->pBatteryLevelCharacteristic = bleKeyboardInstance->pBatteryService->createCharacteristic("2A19", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    bleKeyboardInstance->pBatteryLevelDescriptor = (NimBLE2904*)bleKeyboardInstance->pBatteryLevelCharacteristic->createDescriptor("2904"); 
    bleKeyboardInstance->pBatteryLevelDescriptor->setFormat(NimBLE2904::FORMAT_UTF8);
    bleKeyboardInstance->pBatteryLevelDescriptor->setNamespace(1);
    bleKeyboardInstance->pBatteryLevelDescriptor->setUnit(0x27ad);
    bleKeyboardInstance->pBatteryLevelDescriptor->setCallbacks(&dscCallbacks);


    // remap Input
    bleKeyboardInstance->pInputCharacteristic4 = bleKeyboardInstance->pHidService->createCharacteristic("2A4D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC); // Report
    bleKeyboardInstance->pDesc5 = bleKeyboardInstance->pInputCharacteristic4->createDescriptor( "2908", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC, 20); // Report Reference
    uint8_t desc5_val[] = { REPORT_AZTOOL_ID, 0x01 }; // Report ID 4 を Input に設定
    bleKeyboardInstance->pDesc5->setValue((uint8_t*) desc5_val, 2);
    bleKeyboardInstance->pDesc5->setCallbacks(&RemapDscCallbacks);


    // remap Output
    RemapOutputCallbacks* remapOutputClass = new RemapOutputCallbacks();
    remapOutputClass->pInputCharacteristic = bleKeyboardInstance->pInputCharacteristic4;
    bleKeyboardInstance->pOutputCharacteristic2 = bleKeyboardInstance->pHidService->createCharacteristic("2A4D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC);
    bleKeyboardInstance->pOutputCharacteristic2->setCallbacks(remapOutputClass);
    bleKeyboardInstance->pDesc6 = bleKeyboardInstance->pOutputCharacteristic2->createDescriptor("2908", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::WRITE_ENC, 20);
    uint8_t desc6_val[] = { REPORT_AZTOOL_ID, 0x02}; // Report ID 4 を Output に設定
    bleKeyboardInstance->pDesc6->setValue((uint8_t*) desc6_val, 2);


    /** Start the services when finished creating all Characteristics and Descriptors */
    bleKeyboardInstance->pDeviceInfoService->start();
    bleKeyboardInstance->pHidService->start();
    bleKeyboardInstance->pBatteryService->start();

    bleKeyboardInstance->pAdvertising = NimBLEDevice::getAdvertising();
    bleKeyboardInstance->pAdvertising->setAppearance(HID_KEYBOARD); //HID_KEYBOARD / HID_MOUSE
    bleKeyboardInstance->pAdvertising->addServiceUUID(bleKeyboardInstance->pHidService->getUUID());
    bleKeyboardInstance->pAdvertising->setScanResponse(true);
    bleKeyboardInstance->pAdvertising->start();

    // bleKeyboardInstance->inputKeyboard = pInputCharacteristic;
    // bleKeyboardInstance->inputMediaKeys = pInputCharacteristic2;
    // bleKeyboardInstance->outputKeyboard = pOutputCharacteristic;

    ESP_LOGD(LOG_TAG, "Advertising started!");
    vTaskDelay(portMAX_DELAY); //delay(portMAX_DELAY);
};

unsigned short BleKeyboardJIS::modifiers_press(unsigned short k) {
  this->setConnInterval(0); // 消費電力モード解除
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


unsigned short BleKeyboardJIS::modifiers_release(unsigned short k) {
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

// Shiftを離す
void BleKeyboardJIS::shift_release() {
  int i;
  this->_keyReport.modifiers &= ~(0x22);
  for (i=0; i<6; i++) {
    if (this->_keyReport.keys[i] == 225 || this->_keyReport.keys[i] == 229) {
      this->_keyReport.keys[i] = 0;
    }
  }
}

unsigned short BleKeyboardJIS::modifiers_media_press(unsigned short k) {
  this->setConnInterval(0); // 消費電力モード解除
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

unsigned short BleKeyboardJIS::modifiers_media_release(unsigned short k) {
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

void BleKeyboardJIS::sendReport(KeyReport* keys)
{
  if (this->isConnected())
  {
    this->pInputCharacteristic->setValue((uint8_t*)keys, sizeof(KeyReport));
    this->pInputCharacteristic->notify();
  }
};

void BleKeyboardJIS::sendReport(MediaKeyReport* keys)
{
  if (this->isConnected())
  {
    this->pInputCharacteristic2->setValue((uint8_t*)keys, sizeof(MediaKeyReport));
    this->pInputCharacteristic2->notify();
  }
};

void BleKeyboardJIS::mouse_click(uint8_t b)
{
    this->_MouseButtons = b;
    this->mouse_move(0,0,0,0);
    delay(10);
    this->_MouseButtons = 0x00;
    this->mouse_move(0,0,0,0);
};


void BleKeyboardJIS::mouse_press(uint8_t b)
{
    this->_MouseButtons |= b;
    this->mouse_move(0,0,0,0);
};

void BleKeyboardJIS::mouse_release(uint8_t b)
{
    this->_MouseButtons &= ~(b);
    this->mouse_move(0,0,0,0);
};

void BleKeyboardJIS::mouse_move(signed char x, signed char y, signed char wheel, signed char hWheel)
{
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
        this->pInputCharacteristic3->setValue(m, 5);
        this->pInputCharacteristic3->notify();
    }
};

size_t BleKeyboardJIS::press_raw(unsigned short k)
{
  uint8_t i;
  unsigned short kk;
  this->setConnInterval(0); // 消費電力モード解除
  // メディアキー
  if (modifiers_media_press(k)) return 1;
  ESP_LOGD(LOG_TAG, "press_raw: %D", k);
  kk = this->modifiers_press(k);
  ESP_LOGD(LOG_TAG, "press_raw modifiers: %D", this->_keyReport.modifiers);
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
      ESP_LOGD(LOG_TAG, "press_raw error: %D", kk);
      return 0;
    }
  }
  this->sendReport(&_keyReport);
  return 1;
};

size_t BleKeyboardJIS::press_set(uint8_t k)
{
  uint8_t i;
  unsigned short kk;
  this->setConnInterval(0); // 消費電力モード解除
  kk = _asciimap[k];
  if (!kk) {
    ESP_LOGD(LOG_TAG, "press_set error: %D", k);
    return 0;
  }
  this->_keyReport.modifiers = 0x00;
  kk = this->modifiers_press(kk);
  this->_keyReport.keys[0] = kk;
  this->_keyReport.keys[1] = 0x00;
  this->_keyReport.keys[2] = 0x00;
  this->_keyReport.keys[3] = 0x00;
  this->_keyReport.keys[4] = 0x00;
  this->_keyReport.keys[5] = 0x00;

  this->sendReport(&this->_keyReport);
  return 1;
};

size_t BleKeyboardJIS::release_raw(unsigned short k)
{
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

void BleKeyboardJIS::releaseAll(void)
{
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

// Shiftが押されている状態かどうか(物理的に)
bool BleKeyboardJIS::onShift()
{
  int i;
  for (i=0; i<PRESS_KEY_MAX; i++) {
    if (press_key_list[i].key_num < 0) continue; // 押されたよデータ無ければ無視
    if (press_key_list[i].unpress_time > 0) continue; // 離したよカウントが始まっていたら押していないので無視
    if (press_key_list[i].key_id == 225 || press_key_list[i].key_id == 229) return true; // ShiftコードならばShiftが押されている
  }
  return false;
}

// コネクションインターバル設定
void BleKeyboardJIS::setConnInterval(int interval_type)
{
  if (hid_power_saving_mode == 0) return; // 通常モードなら何もしない
  hid_state_change_time = millis() + hid_saving_time;
  if (hid_power_saving_state == interval_type) return; // ステータスの変更が無ければ何もしない
  hid_power_saving_state = interval_type;
  if (hid_interval_saving == hid_interval_normal) return; // 省電力モードのインターバルと通常モードのインターバルが一緒なら何もしない
  if (!this->isConnected()) return; // 接続していなければ何もしない
  if (interval_type == 1) {
    // 省電力中
    this->pServer->updateConnParams(hid_conn_handle, hid_interval_saving - 2, hid_interval_saving + 2, 0, 200);
  } else {
    // 通常
    this->pServer->updateConnParams(hid_conn_handle, hid_interval_normal - 2, hid_interval_normal + 2, 0, 200);
  }
    
}

#endif // KEYBOARD_TYPE == 0
