// Fan motor speed depends on fastest => 255 | slowest => 0

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

int motorPin = 14;   // D5 send signal to control fan
int motorSpeed = 0;     // motor speed variable
const int motorFullSpeed = 255; // motor speed value at full speed
const int motorStopSpeed = 0; // motor speed value at stop speed

// Set Wiff access information
const char* ssid = "where is your dick"; // wifi name
const char* password = "boyulinzuishuai"; // wifi password

// MQTT connection config
// const char* mqttServer = "test.ranye-iot.net";  // mqtt server address
const char* mqttServer = "139.224.56.89";  // mqtt server address
const int mqttPort = 1883;                      // mqtt server port

const char* mqttUserName = "user";    // 服务端连接用户名(需要修改)
const char* mqttPassword = "123456";  // 服务端连接密码(需要修改)

String clientIdStr = "esp8266-" + WiFi.macAddress();
const char* clientId = clientIdStr.c_str();  // 客户端id (需要修改)

// const char* subTopic = "iotuser/led_kz";        // 订阅主题(需要修改)
// const char* pubTopic = "iotuser/led_zt";        // 订阅主题(需要修改)
// const char* willTopic = "iotuser/led_yz";       // 遗嘱主题名称(需要修改)

//遗嘱相关信息
// const char* willMsg = "esp8266 offline";        // 遗嘱主题信息
// const int willQos = 0;                          // 遗嘱QoS
// const int willRetain = false;                   // 遗嘱保留

// const int subQoS = 1;            // 客户端订阅主题时使用的QoS级别（截止2020-10-07，仅支持QoS = 1，不支持QoS = 2）
// const bool cleanSession = false; // 清除会话（如QoS>0必须要设为false）

Ticker ticker;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

int count;    //  variable that used in Ticker count time(sec)

void setup()
{
  Serial.begin(9600);

  // Ticker定时对象
  ticker.attach(1, tickerCount); // invoke the recall function tickerCount every second

  // Set Motor signal pin to output mode
  pinMode(motorPin, OUTPUT);

  // Set ESP8266 work mode to wireless terminal mode
  WiFi.mode(WIFI_STA);

  // Connect Wifi
  connectWifi();

  // Set MQTT server and port
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(receiveCallback);

  // Connect MQTT Server
  connectMQTTServer();
}

void loop()  { 
  // keep connect heartbeat mechanism
  if (mqttClient.connected()) { // if the board connect the server successfully
    // publish message every 5 seconds
    if (count >= 5){
      pubMQTTmsg();
      count = 0;
    }    
    // keep heart beat
    mqttClient.loop();
  } else {                  // if the connection between board and server is failed
    connectMQTTServer();    // then try to connect to the server
  }                          
}

void tickerCount(){
  count++;
}

// Fan at full speed
void fullSpeedFan() {
  motorSpeed = motorFullSpeed;
  controlFan(motorSpeed);
}
// Fan at stop speed
void closeFan() {
  motorSpeed = motorStopSpeed;
  controlFan(motorSpeed);
}

// Control fan speed precisely
// 0 <= speed <= 255
void controlFan(int newSpeed) {
    analogWrite(motorPin, newSpeed);   // PWM control speed
    // publish the device's state after changing 
    pubMQTTmsg();
    delay(30);
}

// ESP8266 connect MQTT server
void connectMQTTServer(){
  // according to ESP8266's mac address to generate client device ID (to avoid ID collision)
  // String clientId = "esp8266-" + WiFi.macAddress();

  // connect MQTT server
  // Serial.println("MQTT connection status: ");
  // Serial.println(String(mqttClient.connect(clientId.c_str())));
  // Serial.println(String(mqttClient.connect(clientId)));
  /* 连接MQTT服务器
  boolean connect(const char* id, const char* user, 
                  const char* pass, const char* willTopic, 
                  uint8_t willQos, boolean willRetain, 
                  const char* willMessage, boolean cleanSession); 
  若让设备在离线时仍然能够让qos1工作，则connect时的cleanSession需要设置为false                
  */
  if (mqttClient.connect(clientId, mqttUserName, mqttPassword)) {
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address: ");
    Serial.println(mqttServer);
    Serial.println("ClientId:");
    // Serial.println(clientId);
    Serial.println(clientIdStr);
    subscribeTopic();  // subscribe to specific topic
  } else {
    Serial.print("MQTT Server Connect Failed. Client State:");
    Serial.println(mqttClient.state());
    delay(3000);
  }   
}

// Publish autoFan status (On / Off)
void pubLightStatus() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "autoFan-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the state of the autoLight
  String messageString;
  if (motorSpeed > 0) {
    messageString = "On";
  } else {
    messageString = "Off";
  }
  char publishMsg[messageString.length() + 1];
  strcpy(publishMsg, messageString.c_str());

  // achieve to publish message to MQTT server through ESP8266
  if (mqttClient.publish(publishTopic, publishMsg)) {
    Serial.println("Publish Topic:");
    Serial.println(publishTopic);
    Serial.println("Publish message:");
    Serial.println(publishMsg);
  } else {
    Serial.println("Message Publish Failed.");
  }
}

// Publish autoFan connection status (Online / Offline)
void pubLightConnectionStatus() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "autoFan-connection-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the connection state of the autoLight
  String messageString = "Online";

  char publishMsg[messageString.length() + 1];
  strcpy(publishMsg, messageString.c_str());

  // achieve to publish message to MQTT server through ESP8266
  if (mqttClient.publish(publishTopic, publishMsg)) {
    Serial.println("Publish Topic:");
    Serial.println(publishTopic);
    Serial.println("Publish message:");
    Serial.println(publishMsg);
  } else {
    Serial.println("Message Publish Failed.");
  }
}

// Publish autoFan power Degree
void pubLightPowerDegree() {
  // according to ESP8266's mac address to generate client device ID
  // (to avoid ID collision) when different users publish messages
  String topicString = "autoFan-power-status-" + WiFi.macAddress();
  char publishTopic[topicString.length() + 1];
  strcpy(publishTopic, topicString.c_str());

  // publish the state of the autoFan
  String messageString;

  if (motorSpeed == 255) {
    messageString = "100";
  } 
  if (motorSpeed == 0) {
    messageString = "0";
  }

  char publishMsg[messageString.length() + 1];
  strcpy(publishMsg, messageString.c_str());

  // achieve to publish message to MQTT server through ESP8266
  if (mqttClient.publish(publishTopic, publishMsg)) {
    Serial.println("Publish Topic:");
    Serial.println(publishTopic);
    Serial.println("Publish message:");
    Serial.println(publishMsg);
  } else {
    Serial.println("Message Publish Failed.");
  }
}

// Publish messages
void pubMQTTmsg(){
  pubLightStatus();
  pubLightConnectionStatus();
  pubLightPowerDegree();
}

// subscribe autoFan
void subscribeAutoLightTopic() {
  // Create subscrib topic with postfix with Mac address
  // to avoid topic collision when similar devices connect to the same devices
  String topicString = "autoFan-" + WiFi.macAddress();
  char subTopic[topicString.length() + 1]; // plus one is a settled operation, you can see as the string has a hide one at the end
  strcpy(subTopic, topicString.c_str());
  
  // Through the serial monitor to check if subscribe the topic successfully and the name info about this topic
  // method subscribe beneath require arguments to be a char array
  if(mqttClient.subscribe(subTopic)){
    Serial.println("Subscribe Topic:");
    Serial.println(subTopic);
  } else {
    Serial.print("Subscribe Fail...");
  }  
}

// Subscrib specific topic
// 后续改进 能不能通过外部传参 topic的前缀来制定订阅的topic名字
void subscribeTopic(){
  subscribeAutoLightTopic();
}

// autoFan callback
void autoFanCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message Received [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
  Serial.print("Message Length(Bytes) ");
  Serial.println(length);
 
  if ((char)payload[0] == '1') {     // if the message received started with 1
    fullSpeedFan(); // Then fan is opened.
  } else {                           
    closeFan(); // Otherwise the fan is closed.
  }
}

// Callback after receive message from MQTT server
void receiveCallback(char* topic, byte* payload, unsigned int length) {
  String comingTopic = topic;
  String autoFanTopicString = "autoFan-" + WiFi.macAddress();
  // autoFan callback
  if (comingTopic == autoFanTopicString) {
    autoFanCallback(topic, payload, length);
  }
}

// ESP8266 connect wifi
void connectWifi() {
  WiFi.begin(ssid, password);

  // Wait until the connection success, then output the successful message
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("");
}