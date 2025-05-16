#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SERVICE_UUID        "91bad492-b950-4226-aa2b-4ede9fa42f59"
#define CHARACTERISTIC_UUID "0d563a58-196a-48ce-ace2-dfec78acc814"

static BLEAdvertisedDevice* myDevice;
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
BLEClient* pClient;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("서버에 연결됨");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("서버 연결이 끊어졌습니다.");
  }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("광고 중인 BLE 장치 발견: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      Serial.println("원하는 BLE 서버를 찾았습니다!");
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

bool connectToServer() {
  pClient = BLEDevice::createClient();
  Serial.println("BLE 클라이언트 생성됨");

  pClient->setClientCallbacks(new MyClientCallback());

  pClient->connect(myDevice);
  Serial.println("서버에 연결 시도 중...");

  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("BLE 서비스가 없습니다.");
    pClient->disconnect();
    return false;
  }

  BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("BLE 캐릭터리스틱이 없습니다.");
    pClient->disconnect();
    return false;
  }

  if(pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify([](BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
      std::string value = std::string((char*)pData, length);
      Serial.print("수신된 데이터: ");
      Serial.println(value.c_str());

      // OLED 출력
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 10);
      display.println("ble: ");
      display.setCursor(0, 30);
      display.println(value.c_str());
      display.display();
    });
  }

  connected = true;
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("BLE 클라이언트 시작 중...");

  // OLED 초기화
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED 초기화 실패");
    for(;;); // 무한 루프
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,10);
  display.println("BLE 클라이언트 준비됨");
  display.display();

  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("서버 연결 성공");
    } else {
      Serial.println("서버 연결 실패");
    }
    doConnect = false;
  }

  if (connected) {
    // 연결되어 있으면 루프 안에서 특별한 작업은 필요 없음
  } else if (doScan == true) {
    BLEDevice::getScan()->start(0); // 무한 스캔
  }

  delay(1000);
}



