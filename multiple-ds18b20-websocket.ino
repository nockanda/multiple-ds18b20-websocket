
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>
//아래에 있는 내용은 당연히 있어야한느 것이다!
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D3 //몇번핀에 연결했냐
#define TEMPERATURE_PRECISION 12
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress myaddress1,myaddress2,myaddress3;
DeviceAddress mysensors[]={
  {0x28,0x92,0x4B,0x79,0xA2,0x00,0x03,0x38},
  {0x28,0xA9,0x4B,0x95,0xF0,0x01,0x3C,0x79},
  {0x28,0xE2,0xEA,0x95,0xF0,0x01,0x3C,0x0A}
};
//mysensors[0]
//mysensors[1]
//mysensors[2]

float mytemp[3] = {0,0,0};

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;

//웹서버의 포트는 기본 포트인 80번을 사용한다!
ESP8266WebServer server(80);
//웹서버와 웹클라이언트가 뒷구멍으로 주고받는 데이터는 웹소켓에서 81번을 쓴다!
WebSocketsServer webSocket = WebSocketsServer(81);

String response = "\
<html>\
<head>\
<meta name=\"viewport\" content=\"width=device-width\">\
<meta charset=\"utf-8\">\
<script>\
  var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);\
  connection.onopen = function () {\
     connection.send('Connect ' + new Date());\
  };\
  connection.onerror = function (error) {\
     console.log('WebSocket Error ', error);\
  };\
  connection.onmessage = function (e) {\
     console.log('Server: ', e.data);\
     var mydata = JSON.parse(e.data);\
     document.getElementById('temp1').innerText = mydata.temp1 +\"'C\";\
     document.getElementById('temp2').innerText = mydata.temp2 +\"'C\";\
     document.getElementById('temp3').innerText = mydata.temp3 +\"'C\";\
  };\
</script>\
</head>\
<body>\
<table width=500 height=300 border=1>\
  <tr>\
     <th colspan=2><h1>녹칸다 포에버</h1></th>\
  </tr>\
  <tr>\
      <th>커피머신</th>\
      <td align=center id=temp1>-'C</td>\
  </tr>\
  <tr>\
      <th>온수매트</th>\
      <td align=center id=temp2>-'C</td>\
  </tr>\
  <tr>\
      <th>보일러온수</th>\
      <td align=center id=temp3>-'C</td>\
  </tr>\
</table>\
</body>\
</html>";


//클라이언트에서 서버쪽으로 값이 전송되었을때 뭘할거냐?
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            //num = 소켓번호(클라이언트번호)
            //webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
        //메시지 수신부
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);


            break;
    }

}

void setup() {
    //USE_SERIAL.begin(921600);
    USE_SERIAL.begin(115200);
    sensors.begin(); //있어야한느거
    if(sensors.getDeviceCount() == 3){
      //발견한 주소를 대입한다!
      sensors.getAddress(myaddress1, 0);  
      sensors.getAddress(myaddress2, 1);  
      sensors.getAddress(myaddress3, 2);
  
      sensors.setResolution(myaddress1, TEMPERATURE_PRECISION);
      sensors.setResolution(myaddress2, TEMPERATURE_PRECISION);
      sensors.setResolution(myaddress3, TEMPERATURE_PRECISION);
    }else{
      //Serial.println("연결되어있지 않거나 2개이상의 센서가 연결되었다!");
    }
    
    //USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    //자기자신의 IP공유기 ID비밀번호 집어넣는곳!
    WiFiMulti.addAP("popcorn", "11213144");

    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }

    //IP공유기로부터 할당받은 IP주소를 여기서 출력한다!
    USE_SERIAL.println("IP address: ");
    USE_SERIAL.println(WiFi.localIP());
  
    //웹소켓 서버를 연다
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    //윈도우10, 안드로이드 안됨..의미없는 기능
    if(MDNS.begin("esp8266")) {
        USE_SERIAL.println("MDNS responder started");
    }

    //웹서버의 index.html
    //웹서버가 클라이언트에게 response해주는 부분!

    server.on("/", []() {
        server.send(200, "text/html", response);
    });

    server.begin();

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
}

unsigned long last_10sec = 0;
unsigned int counter = 0;

void loop() {
    unsigned long t = millis();
    webSocket.loop(); //이거 있어야한다!
    server.handleClient(); //이거도 있어야한다!

    //delay(~~~~) 절때 쓰면 안됨!
/*
    //10초간격으로 뭔가 하겠다~
    if((t - last_10sec) > 1000) {
      last_10sec = millis();
      String msg = "현재 사물인터넷보드의 시간="+String(millis());
      webSocket.broadcastTXT(msg); //모든클라이언트에게 메시지 전송!
        
    }
    */
    sensors.requestTemperatures();
  //Serial.println("DONE");

  printTemperature(myaddress1);
  printTemperature(myaddress2);
  printTemperature(myaddress3);

  String myjson = "{\"temp1\":"+String(mytemp[0])+",\"temp2\":"+String(mytemp[1])+",\"temp3\":"+String(mytemp[2])+"}";
  webSocket.broadcastTXT(myjson);
}
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }

  //지금 입력받은 주소로 온도값을 측정하는데 누구껀지 확인해보자!
  for(int i =0;i<3;i++){
    if(is_match(deviceAddress,mysensors[i])){
       //match
       //Serial.print(i+1);
       //Serial.println("번째 센서입니다!");
       mytemp[i] = tempC;
       break;
    }
  }
  
  //Serial.print("Temp C: ");
  //Serial.println(tempC);
}

//주소2개를 입력받아서 같은지 아닌지를 반환하는 함수!
bool is_match(DeviceAddress da1, DeviceAddress da2){
  //일단 같다고 보고 시작한다!
  bool result = true;
  for(int i = 0;i<8;i++){
    //단 하나라도 틀리면 틀린것이다!
    if(da1[i] != da2[i]){
      result = false;
      break;
    }
  }

  return result;
}
