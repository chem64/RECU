/*  RECU_BLE
 *  Chester Marshall 2021
 *  This program will read Jeep Renix ECU codes and export via bluetooth low energy (BLE) to an Android app
 *  based on work by John Eberle (circuit), Nick Risley (ECU Frame and circuit), Phil Andrews (original proof of concept)
 */
uint8_t ver = 105;
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
 *         1.03  - 2/14/2021
 *               - cleaned up unused functions and variables
 *               - limited frame and test counts to 127
 *         1.04  - there is a bug in frame handling introduced in 1.02 - reverted to previous frame handler from 1.01 (with new buffer size)
 *               - removed LED toggle in frame handler - now LED is only for connections 
 *               - framebuffer(31) is for bad frame count
 *         1.05  - now using BLE notify in addition to BLE read, so either method can be used       
 *         
 */
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
BLECharacteristic *pChar1;
BLECharacteristic *pChar2;
BLEAdvertising *pAdvertising;
#define SERVICE_UUID        "13a63cfd-fde2-42aa-8d59-6658a5031063"  
#define CHARACTERISTIC_UUID "e922f584-83a4-4317-a384-41248aeaf139"  //40 bytes for reads
#define CHAR1_UUID "1922f584-83a4-4317-a384-41248aeaf139"           //0-19 bytes for notify
#define CHAR2_UUID "2922f584-83a4-4317-a384-41248aeaf139"           //20-34 bytes for notify

uint8_t FRAME_SIZE = 28;          //ECU Frame size constant
byte frameBuffer[35];
byte char2Buffer[20];
uint16_t bufferPosition = 0;     //Current reception byte position
bool frameCompleteFlag = false;  //Set when a frame is ready
bool _0xFFReceived = false;       //Set when a 0xFF received
bool bufferFilling = false;      //Indicates a frame reception is in progress
uint8_t lastByte = 0;            //Tracks last byte received
uint8_t bufferError = 0;         //Indicates a buffer error
uint8_t badFrames = 0;          //Indicates number of bad Comm Frames from ECU
uint8_t FrameCount = 0;         //rolling count of good frames
uint8_t testCount = 0;          //test count for when not connected to ECU
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

void SetLed(bool onState)
{
  digitalWrite(2, onState);    // If HIGH, then turn LED on.
}

void NotifyData()
{
  if (deviceConnected) 
  {
    pCharacteristic->setValue(frameBuffer,35);
    pChar1->setValue(frameBuffer,20);
    pChar1->notify();
    delay(10);
    char2Buffer[0] = frameBuffer[20];
    char2Buffer[1] = frameBuffer[21];
    char2Buffer[2] = frameBuffer[22];
    char2Buffer[3] = frameBuffer[23];
    char2Buffer[4] = frameBuffer[24];
    char2Buffer[5] = frameBuffer[25];
    char2Buffer[6] = frameBuffer[26];
    char2Buffer[7] = frameBuffer[27];
    char2Buffer[8] = frameBuffer[28];
    char2Buffer[9] = frameBuffer[29];
    char2Buffer[10] = frameBuffer[30];
    char2Buffer[11] = frameBuffer[31];
    char2Buffer[12] = frameBuffer[32];
    char2Buffer[13] = frameBuffer[33];
    char2Buffer[14] = frameBuffer[34];
    pChar2->setValue(char2Buffer,15);
    pChar2->notify();
  }
}

void ManageSerialReceive()
{
  uint8_t byte_read;

  while( Serial2.available() > 0)  //Loop while there's anything in the serial buffer
  {
     byte_read = Serial2.read();  //Read any data received (at a minimum empty the buffer))
     ReceiveDataFrame(byte_read); //Send byte read to be put in dataframe
     if(frameCompleteFlag)       //If we have a complete dataframe
     {
        if(bufferError == 0)     //If there was not an error receiving frame
        {
          ECUdataFlag = true;
          FrameCount++;
          if(FrameCount > 127) FrameCount = 0;
          frameBuffer[33] = FrameCount;
          NotifyData();
        }
        else
        {
          ++badFrames;
          if(badFrames > 127) badFrames = 0;
          frameBuffer[31] = badFrames;
          NotifyData();
        }
        frameCompleteFlag = false;   //Reset Flag to start new receive
     }
  }
}

bool ReceiveDataFrame(uint8_t data)
{
    bool storeByteFlag = false; //Indicates we should write the byte into the buffer
    uint8_t byteData;
    if (frameCompleteFlag)      //Unread frame waiting - ignore data
        return false;
    byteData = data;
    if(!bufferFilling)          //Waiting on 0xFF, 0x00 sequence to start
    {  
        if((byteData == 0x00) && (lastByte == 0xFF))   //if current byte is 0x00 and last was 0xFF then start frame
        {
            bufferFilling = true;   //Indicate filling in progress
            bufferPosition = 0;     //Reset frame fill position
            bufferError = 0;        //reset error
            lastByte = 0;           //reset for next frame start
            _0xFFReceived = false;  //Reset double 0xFF data indicator
        }
        else
            lastByte = byteData;    //Not start of frame, so keep byte for next check
    }
    else
    {   //Start of Frame received so Receiving Data
        if(_0xFFReceived)
        {   //Last Byte into buffer was a 0xFF
            switch(byteData)
            {
                case 0xFF:
                    //We have a 0xFF followed by a 0xFF.  This indicates that the prior value
                    //was valid (and not start of new frame transmission).  Simply discard this byte
                    //by not setting the store storeByteFlag.
                    break;
                case 0:
                    //We have a 0xFF followed by a 0x00. Treat as start of new Frame transmission
                    bufferError = 2;       //Indicate Short Count On Buffer
                    --bufferPosition;      //Back up to last good data
                    bufferFilling = false; //Stop Filling
                    frameCompleteFlag = true;
                    break;
                default:
                    //Should not happen.  Assume Data is bad and reset buffer
                    bufferFilling = false;
                    break;
            }
            _0xFFReceived = false;  //Reset the flag
        }
        else
        {
            storeByteFlag = true;   //Prior byte was not 0xFF.  Store it and set flag if current byte is 0xFF
            if (byteData == 0xFF) _0xFFReceived = true;
        }
    }
    //If there is valid data to store
    if (storeByteFlag)
    {
        frameBuffer[bufferPosition++] = byteData; //Store the data
        if(bufferPosition >= FRAME_SIZE)   //Full frame so indicate
        {
            bufferFilling = false; //Stop Filling
            frameCompleteFlag = true;
        }
    }
    return true;
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
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID,BLECharacteristic::PROPERTY_READ);
  pCharacteristic->addDescriptor(new BLE2902());
  pChar1 = pService->createCharacteristic(CHAR1_UUID,
    BLECharacteristic::PROPERTY_READ | 
    BLECharacteristic::PROPERTY_NOTIFY | 
    BLECharacteristic::PROPERTY_INDICATE);
  pChar1->addDescriptor(new BLE2902());
  pChar2 = pService->createCharacteristic(CHAR2_UUID,
    BLECharacteristic::PROPERTY_READ | 
    BLECharacteristic::PROPERTY_NOTIFY | 
    BLECharacteristic::PROPERTY_INDICATE);
  pChar2->addDescriptor(new BLE2902());
  frameBuffer[34] = ver;
  pService->start();
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  //pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  //pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void loop()
{
  if (!deviceConnected && oldDeviceConnected) 
  {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) 
  {
    ECUdataFlag = false;
    timerLast = millis();
    oldDeviceConnected = deviceConnected;
  }

  if(deviceConnected) 
  {
    if(ECUdataFlag == false)
    {
      long timerNow = millis();
      if (timerNow - timerLast > 250)          
      {
        timerLast = timerNow;
        testCount++;
        if(testCount > 127) testCount = 0;
        frameBuffer[4] = testCount;
        frameBuffer[24] = testCount;
        frameBuffer[32] = testCount;  
        NotifyData();
      }
    }  
  }
  ManageSerialReceive();  
  delay(50);
}
