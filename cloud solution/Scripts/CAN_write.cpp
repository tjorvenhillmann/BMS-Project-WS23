#include <mcp2515.h>
#include <Arduino.h>
#include <SPI.h>

float temp_1 = 37.12;
float temp_2 = 27.12;
float temp_3 = 47.12;
float temp_4 = 57.12;

float cell_1_v = 4.123;
float cell_2_v = 3.789;
float cell_3_v = 3.156;
float cell_4_v = 4.200;

float current = 2.546;
float SOC = 0.45;
float SOH = 0.34;
float capacity = 0.89;

byte balancer_status = 0;

bool balanc_status_cell_1 = 1;
bool balanc_status_cell_2 = 0;
bool balanc_status_cell_3 = 1;
bool balanc_status_cell_4 = 0;

bool cloud_status = 1;

struct can_frame canMsg1;
struct can_frame canMsg2;
struct can_frame canMsg3;
struct can_frame canMsg4;

MCP2515 mcp2515(10);

void float2byte(float x, byte *var, int resolution){
  *var = (int)(x*resolution) & 0xff;
  *(var+1) = (int)(x*resolution) >> 8 & 0xff;
}

void construct_canmsg_1(){
  byte msg[2] = {0,0};
  canMsg1.can_id  = 0x001;
  canMsg1.can_dlc = 8;
  float2byte(temp_1,msg,100);
  canMsg1.data[0] = msg[0];
  canMsg1.data[1] = msg[1];
  float2byte(temp_2,msg,100);
  canMsg1.data[2] = msg[0];
  canMsg1.data[3] = msg[1];
  float2byte(temp_3,msg,100);
  canMsg1.data[4] = msg[0];
  canMsg1.data[5] = msg[1];
  float2byte(temp_4,msg,100);
  canMsg1.data[6] = msg[0];
  canMsg1.data[7] = msg[1];
}

void construct_canmsg_2(){
  byte msg[2] = {0,0};
  canMsg2.can_id  = 0x002;
  canMsg2.can_dlc = 8;
  float2byte(cell_1_v,msg,1000);
  canMsg2.data[0] = msg[0];
  canMsg2.data[1] = msg[1];
  float2byte(cell_2_v,msg,1000);
  canMsg2.data[2] = msg[0];
  canMsg2.data[3] = msg[1];
  float2byte(cell_3_v,msg,1000);
  canMsg2.data[4] = msg[0];
  canMsg2.data[5] = msg[1];
  float2byte(cell_4_v,msg,1000);
  canMsg2.data[6] = msg[0];
  canMsg2.data[7] = msg[1];
}

void construct_canmsg_3(){
  byte msg[2] = {0,0};
  canMsg3.can_id  = 0x003;
  canMsg3.can_dlc = 8;
  float2byte(current,msg,1000);
  canMsg3.data[0] = msg[0];
  canMsg3.data[1] = msg[1];
  float2byte(SOC,msg,10000);
  canMsg3.data[2] = msg[0];
  canMsg3.data[3] = msg[1];
  float2byte(SOH,msg,10000);
  canMsg3.data[4] = msg[0];
  canMsg3.data[5] = msg[1];
  float2byte(capacity,msg,10000);
  canMsg3.data[6] = msg[0];
  canMsg3.data[7] = msg[1];
}

void construct_canmsg_4(){
  byte msg[2] = {0,0};
  canMsg4.can_id  = 0x004;
  canMsg4.can_dlc = 1;
  balancer_status = balanc_status_cell_1;
  balancer_status = balanc_status_cell_2 >> 1 & 0xff;
  balancer_status = balanc_status_cell_3 >> 2 & 0xff;
  balancer_status = balanc_status_cell_4 >> 3 & 0xff;
  balancer_status = cloud_status >> 4 & 0xFF;
  canMsg4.data[0] = balancer_status;
}

void setup() {
 
  while (!Serial);
  Serial.begin(115200);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  //Serial.println("Example: Write to CAN");
}

void loop() {
  construct_canmsg_1();
  construct_canmsg_2();
  construct_canmsg_3();
  construct_canmsg_4();
  mcp2515.sendMessage(&canMsg1);
  delay(125);
  mcp2515.sendMessage(&canMsg2);
  delay(125);
  mcp2515.sendMessage(&canMsg3);
  delay(125);
  mcp2515.sendMessage(&canMsg4);
  delay(125);
  Serial.println("Messages sent");
  delay(4500);
}
