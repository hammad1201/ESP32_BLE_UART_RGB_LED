/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
float txValue = 0;
const int readPin = 32; // Use GPIO number. See ESP32 board pinouts
const int LED = 2; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.
bool dataReceived = false;
bool sendConnectOk = false;
int i=0;

String input;
long _time;

int red = 18;
int green = 13;
int blue = 14;
// the number of the LED pin

// setting PWM properties
const int freq = 5000;
const int redLedChannel = 0;
const int greenLedChannel = 1;
const int blueLedChannel = 2;
const int resolution = 8;

StaticJsonDocument<200> doc;


//std::string rxValue; // Could also make this a global var to access it in loop()

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

void showColor(int redDutyCycle, int greenDutyCycle, int blueDutyCycle){

  ledcWrite(redLedChannel, 255 - redDutyCycle);
  ledcWrite(greenLedChannel, 255 - greenDutyCycle);
  ledcWrite(blueLedChannel, 255 - blueDutyCycle);
  
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      delay(250);
      Serial.println("Connected");
      sendConnectOk = true;
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Disconnected");
      dataReceived = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {

        deserializeJson(doc, rxValue);
        JsonObject obj = doc.as<JsonObject>();
        
        int redDutyCycle = doc["red"];
        int greenDutyCycle = doc["green"];
        int blueDutyCycle = doc["blue"];
        showColor(redDutyCycle, greenDutyCycle, blueDutyCycle);

        Serial.print("RED::");
        Serial.println(redDutyCycle);

        Serial.print("GREEN::");
        Serial.println(greenDutyCycle);

        Serial.print("BLUE::");
        Serial.println(blueDutyCycle);
        
        
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);          
        }
        Serial.println();
        Serial.println();
        Serial.println("*********");
      }
      dataReceived = true;
      
    }
    
};

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  ledcSetup(redLedChannel, freq, resolution);
  ledcSetup(greenLedChannel, freq, resolution);
  ledcSetup(blueLedChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(red, redLedChannel);
  ledcAttachPin(green, greenLedChannel);
  ledcAttachPin(blue, blueLedChannel);

  // Create the BLE Device
  BLEDevice::init("ESP32 UART Test"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    txValue = 1; 

    // Let's convert the value to a char array:
    char txString[8]; // make sure this is big enuffz
    dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer
   
  delay(1000);
  }
}
