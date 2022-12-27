#ifndef BleCallbacks_h
#define BleCallbacks_h
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "nimconfig.h"
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include <NimBLEServer.h>
#include <NimBLEDevice.h>
#include "HIDKeyboardTypes.h"
#include "HIDTypes.h"
#include "NimBLECharacteristic.h"

#include "hid_common.h"
#include "../../az_common.h"



// Characteristic コールバック クラス
class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
  public:
    CharacteristicCallbacks();
    void onRead(NimBLECharacteristic* pCharacteristic);
    void onWrite(NimBLECharacteristic* pCharacteristic);
    void onNotify(NimBLECharacteristic* pCharacteristic);
    void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code);
    void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue);
};

// Descriptor コールバック クラス
class DescriptorCallbacks : public NimBLEDescriptorCallbacks {
  public:
    DescriptorCallbacks();
    void onWrite(NimBLEDescriptor* pDescriptor);
    void onRead(NimBLEDescriptor* pDescriptor);
};

// コネクションクラス
class BleConnectionStatus : public NimBLEServerCallbacks {
  public:
    BleConnectionStatus(void);
    bool connected = false;
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc);
    void onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc);
};

// Output コールバック クラス
class KeyboardOutputCallbacks : public NimBLECharacteristicCallbacks {
  public:
    KeyboardOutputCallbacks(void);
    void onWrite(NimBLECharacteristic* me);
};


// Remap Descriptor コールバック クラス
class RemapDescriptorCallbacks : public NimBLEDescriptorCallbacks {
  public:
    RemapDescriptorCallbacks();
    void onWrite(NimBLEDescriptor* pDescriptor);
    void onRead(NimBLEDescriptor* pDescriptor);
};


// REMAP Output コールバック クラス
class RemapOutputCallbacks : public NimBLECharacteristicCallbacks {
  public:
	NimBLECharacteristic* pInputCharacteristic; // REMAP 送信用
    RemapOutputCallbacks(void);
    void onWrite(NimBLECharacteristic* me);
	void sendRawData(uint8_t *data, uint8_t data_length); // Remapにデータを返す
};

// HidrawCallback
void HidrawCallbackExec(int data_length);

#endif // CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#endif // CONFIG_BT_ENABLED
#endif
