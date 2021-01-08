#include <WiFi.h>
#include <WiFiUdp.h>
#include "Sensors.h"
#include "Options.h"

const char *ssid = "ap_bird";
const char *password = "lm123456";
const IPAddress serverIP(192,168,4,1); //欲訪問的地址
unsigned int localUdpPort = 6188; //伺服器埠號

#define JOYSTICK_Y 35
#define JOYSTICK_X 34
#define JOYSTICK_RANGE 33
#define JOYSTICK_CALIBRATION 32
#define JOYSTICK_SWITCH 39
#define JOYSTICK_DIRECTION 36

uint16_t pos_y = 0;
int16_t pos_x = 0;
int16_t offset_x = 0;
float ratio_x = 0;

String data = "";

WiFiUDP Udp;  

void setup() {
  Serial.begin(115200);
  Serial.println();
/*
  pinMode(JOYSTICK_Y, INPUT);
  pinMode(JOYSTICK_X, INPUT);
  pinMode(JOYSTICK_RANGE, INPUT);
  pinMode(JOYSTICK_CALIBRATION, INPUT);
  pinMode(JOYSTICK_SWITCH, INPUT);
  pinMode(JOYSTICK_DIRECTION, INPUT);
*/
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected"); 
  Serial.print("IP Address:"); 
  Serial.println(WiFi.localIP());

  //delay(5000); // Wait until start. Last chance to upload new sketch!!!
  init_mpu();

  Udp.begin(localUdpPort);
}

void loop() { 
  /* get Y position */
  (map(analogRead(JOYSTICK_Y),0,4095,1000,2300) < 1150)?pos_y=1000:pos_y=map(analogRead(JOYSTICK_Y),0,4095,1000,2300);
  (pos_y >= 2000)?pos_y=2000:pos_y=pos_y;

  /* get X position */
  /* JOYSTICK_SWITCH switch between Joystick control or motion control */
  if (2000 >= analogRead(JOYSTICK_SWITCH))
  {
    genMPU6050Sample();

    pos_x = map(getDeviceAngleX(),-90,90,1000,2000);
  }
  else
  {
    pos_x = map(analogRead(JOYSTICK_X),0,4095,1000,2000);
  }

  /* JOYSTICK_DIRECTION controls rudder direction */
  if (2000 >= analogRead(JOYSTICK_DIRECTION))
  {
    pos_x = map(pos_x,1000,2000,2000,1000);
  }

  /* JOYSTICK_CALIBRATION calibrates the center position of rudder */
  offset_x = map(analogRead(JOYSTICK_CALIBRATION),0,4095,-500,500);
  pos_x = pos_x + offset_x;

  /* adjust the range of rudder */
  ratio_x = (float) ((float) map(analogRead(JOYSTICK_RANGE),0,4095,0,100))/100;
  pos_x = (pos_x - 1500) * ratio_x + 1500;

  data = "SRV" + String(pos_x) + String(pos_y) + "1500" + "1500";
  Serial.println(data);

  Udp.beginPacket(serverIP, localUdpPort); //準備傳送資料
  Udp.write((uint8_t*) data.c_str(),data.length()+1); //複製資料到傳送快取
  Udp.endPacket();            //傳送資料

  Serial.print("ratio_x: ");
  Serial.println(ratio_x);

  Serial.print("pos_x: ");
  Serial.println(pos_x);

  Serial.print("pos_y: ");
  Serial.println(pos_y);

  delay(10);
}
