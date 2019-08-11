#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Servo.h>

//Change the AP name and password as required.
#define AP_NAME "WiFiRCController"
#define AP_PASSWORD "qwerty123"
#define PORT 9979

const char* JSON_CONNECTION_STATUS                 = "connection";
const char* JSON_CONNECTION_STATUS_CONNECTED       = "connected";
const char* JSON_CONNECTION_STATUS_DISCONNECTED    = "disconnected";
const char* JSON_CHANNEL_INFO                      = "channelinfo";
const char* JSON_CHANNEL_INDEX                     = "index";
const char* JSON_CHANNEL_VALUE                     = "value";


unsigned int DIRECTION_FORWARD                     = 1;
unsigned int DIRECTION_BACKWARD                    = 2;
unsigned int motor1Direction                       = DIRECTION_FORWARD;
unsigned int motor1Pin1                            = D6;
unsigned int motor1Pin2                            = D8;
unsigned int enable1Pin                            = D5;

unsigned int SERVO_PIN                             = D7;
unsigned int SERVO_MAX                             = 180;
unsigned int SERVO_MIN                             = 0;


long lastServoUpdateTime                           = 0;
long servoMovementDelay                            = 15;

boolean apSetupStatus                              = false;
WiFiServer wifiServer(PORT);
Servo servo;

void serialHeader(char *message){
  Serial.println("===========================");
  Serial.println(message);  
  Serial.println("===========================");  
}

void displayAPDetails(){
  Serial.print("IP Address : ");
  Serial.println(WiFi.softAPIP());
  uint8_t macAddr[6];
  WiFi.softAPmacAddress(macAddr);
  Serial.printf("MAC address : %02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
  Serial.printf("AP Name : %s\n", AP_NAME);
  Serial.printf("AP Password : %s\n\n",AP_PASSWORD);
}

boolean setUpSoftwareAP(int verbose){
  boolean returnVal=false;
  Serial.println("Setting up the software AP");
  displayAPDetails();
  returnVal = WiFi.softAP(AP_NAME, AP_PASSWORD) ? true: false;
  if(verbose){
    if(returnVal)
      Serial.println("Successfully created the Software AP.");
    else
      Serial.println("Failed to create the Software AP.");
  }
  return returnVal;  
}

int getConnectedClientCount(){
  return WiFi.softAPgetStationNum();
}

void setupServer(){
  boolean returnVal;
  Serial.printf("Setting up the TCP Server on port %d\n", PORT);
  wifiServer.begin();
  wifiServer.setNoDelay(true);
}

void setMotorDirection(int motorIndex, int motorDirection){
  switch(motorIndex){
    case 1:
      if(motorDirection == DIRECTION_FORWARD){
        Serial.println("Forward direction");
        digitalWrite(motor1Pin1, HIGH);
        digitalWrite(motor1Pin2, LOW);
      }else{
        Serial.println("Backward direction");
        digitalWrite(motor1Pin1, LOW);
        digitalWrite(motor1Pin2, HIGH);
      }
      break;
  }
}

void setServo(int angle){
  int mappedValue = map(angle, 0, 100, SERVO_MIN, SERVO_MAX);
  Serial.printf("Servo Mapped Value : %d\n",mappedValue);
  servo.write(mappedValue);
}

void setMotorSpeed(int motorIndex, int value){
  int mappedValue = map(value, 0, 100, 150, 1023);
  switch(motorIndex){
    case 1:
      Serial.printf("Mapped Value : %d\n",mappedValue);
      analogWrite(enable1Pin, mappedValue);
      break;
  }
}

void testServo(){
  Serial.println("Testing the servo");
  for (int pos = SERVO_MIN; pos <= SERVO_MAX; pos += 1) {
    servo.write(pos);
    delay(15);
  }  
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  serialHeader("WIFI RC Controller Receiver");
  apSetupStatus = setUpSoftwareAP(true);

  if(apSetupStatus){
    setupServer();
    displayAPDetails();
  }

  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  analogWriteFreq(1000);
  servo.attach(SERVO_PIN);
  testServo();
  //pinMode(enable1Pin, OUTPUT);

  //ledcSetup(pwmChannel, freq, resolution);

  //ledcAttachPin(enablePin, pwmChannel);
}

void loop() {
  if(getConnectedClientCount()>=1){
    WiFiClient client = wifiServer.available();
    StaticJsonDocument<200> jsonObj;
    if (client)
    {
      Serial.println("\n[Client connected]");
      jsonObj.clear();
      jsonObj[JSON_CONNECTION_STATUS] = JSON_CONNECTION_STATUS_CONNECTED;
      serializeJson(jsonObj, client);
      
      client.println();
      while (client.connected())
      {
        if (client.available())
        {
          String jsonString = client.readStringUntil('\r\n');
          Serial.printf("JSON String : %s\n\n",jsonString.c_str());
          jsonObj.clear();
          DeserializationError error = deserializeJson(jsonObj, jsonString);
          if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return;
          }
          Serial.printf("Channel %d | Value %d\n",jsonObj[JSON_CHANNEL_INFO][JSON_CHANNEL_INDEX].as<int>(), jsonObj[JSON_CHANNEL_INFO][JSON_CHANNEL_VALUE].as<int>());
          int channel = jsonObj[JSON_CHANNEL_INFO][JSON_CHANNEL_INDEX].as<int>();
          int value = jsonObj[JSON_CHANNEL_INFO][JSON_CHANNEL_VALUE].as<int>();


          switch(channel){
            case 1:
              setMotorSpeed(1, value);
              break;
            case 2:
              setServo(value);
              break;
            case 3:
              if(value == 100){
                setMotorDirection(1,DIRECTION_BACKWARD);
              }else{
                setMotorDirection(1,DIRECTION_FORWARD);
              }     
          }
      }
      delay(1);
     }
      client.stop();
      Serial.println("[Client disonnected]");
  }
}
}
