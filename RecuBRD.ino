/*  RECU_BLE
 *  Chester Marshall 2021
 *  This program will read Jeep Renix ECU codes and export via bluetooth low energy (BLE) to an Android app
 *  based on work by John Eberle (circuit), Nick Risley (ECU Frame and circuit), Phil Andrews (original proof of concept)
 *  This software has ONLY been tested on a 1989 2.5L Manual Transmission Wrangler
 */
uint8_t ver = 103;
/* Version 1.00  - 1/22/2021
 *               - possible issues with notify
 *               - set framebuffer to 31 to accomodate debug bytes
 *         1.01  - 1/23/2021
 *               - changed debug bytes to uint8_t
 *               - added notify after setvalue
 *         1.02  - 2/9/2021
 *               - removed notify
 *               - changed frame capture function
 *               - set framebuffer size to 35
 *               - should be 29 bytes for 2.5L (framebuffer 0-28)
 *               - should be 31 bytes for 4.0L (framebuffer 0-30)
 *               - framebuffer(31) is a spare
 *               - send testcount in framebuffer(32)  (sent when not connected to ECU)
 *               - send framecount in framebuffer(33) (sent when connected to ECU)
 *               - send firmware version in framebuffer(34)  (set on startup)
 *         1.03  - 2/11/2021
 *               - cleaned up unused functions and variables
 */
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
BLEAdvertising *pAdvertising;
#define SERVICE_UUID        "13a63cfd-fde2-42aa-8d59-6658a5031063"  
#define CHARACTERISTIC_UUID "e922f584-83a4-4317-a384-41248aeaf139"  

byte frameBuffer[35];
uint8_t FrameCount = 0;          //rolling count of good frames
uint8_t testCount = 0;           //test count for when not connected to ECU
bool ECUdataFlag = false;
bool deviceConnected = false;
bool oldDeviceConnected = false;
long timerLast = 0;

class MyServerCallbacks: public BLEServerCallbacks
{
    void onConnect(BLEServer* pServer)
    {
      deviceConnected = true;
      digitalWrite(2, HIGH);
    };

    void onDisconnect(BLEServer* pServer)
    {
      deviceConnected = false;
      digitalWrite(2, LOW);
    }
};

void GetSerialData()
{
  static bool dataFlag;
  static byte lastByte;
  static byte index;
  static byte curState;  
  byte curByte;

  while (Serial2.available() > 0)  
  { 
    curByte = Serial2.read();

    if(lastByte == 255 && curByte == 0) curState = 10;  //START of FRAME
    if(lastByte == 255 && curByte == 255) curState = 30; //DUPLICATE - Discard byte

    switch(curState)
    {
      case 10:  //FRAME START
        index = 0;
        curState = 20;
        if (dataFlag == true) //Have received at least one frame
        {
          ECUdataFlag = true;
          FrameCount++;
          if(FrameCount >= 127) FrameCount = 0;
          frameBuffer[33] = FrameCount;
          if (deviceConnected) pCharacteristic->setValue(frameBuffer,35);
        }
        dataFlag = true;
        break;
      case 20:  //FRAME DATA
        frameBuffer[index] = curByte;
        index++;        
        break;
      case 30:  //DUPLICATE 255
        curState = 20;  //don't store and return to DATA state
        break;
    }
    lastByte = curByte;        
  }
}

void setup()
{
  Serial2.begin(62500, SERIAL_8N1, 18, 21);  
  delay(1000);
  //setup blue LED
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  BLEDevice::init("RECU");
  uint16_t mtu = 40;
  BLEDevice::setMTU(mtu);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY | 
                      BLECharacteristic::PROPERTY_INDICATE);
  pCharacteristic->addDescriptor(new BLE2902());
  frameBuffer[34] = ver;
  pCharacteristic->setValue(frameBuffer,35);
  pService->start();
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  //pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue?
  //pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void loop()
{
  if(deviceConnected) 
  {
    if(ECUdataFlag == false)
    {
      long timerNow = millis();
      if (timerNow - timerLast > 1000)          
      {
        timerLast = timerNow;
        testCount++;
        if(testCount > 254) testCount = 0;
        frameBuffer[32] = testCount;  
        pCharacteristic->setValue(frameBuffer,35);
      }
    }  
  }

  if (!deviceConnected && oldDeviceConnected) 
  {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) 
  {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
  
  GetSerialData();
  delay(50);
}
