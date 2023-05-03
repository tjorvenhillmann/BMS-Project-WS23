#include <SPI.h>
#include <mcp2515.h>

MCP2515 mcp2515(49); // CS PIN from MCP CAN Module

struct can_frame canMsg1;
struct can_frame canMsg2;
struct can_frame canMsg3;
struct can_frame canMsg4;
int resolution = 100;
byte frame_id_1 = 0x001;
byte frame_id_2 = 0x002;
byte frame_id_3 = 0x003;
byte frame_id_4 = 0x004;
int can_counter=0;
float temp_list[] = {37.12, 20.20, 41.10,15.78}; // E80, 7E4, 100E, 62A
float vol_list[] = {8.98, 10.12, 5.75, 7.89};   // 382, 3F4, 23F, 315
float current_soc_soh[] = {};
byte balance_list[] = {1,0,1,1};

byte tmsg[2] = {0x00,0x00};
  // example temp convertion: 
  // T = 25.27 celcius
  // T * 100  => 2527
  // T to He  => 0x09DF

void setup() {
  canMsg1.can_id  = frame_id_1;  // Temperature ID: 001
  canMsg1.can_dlc = 8;

  canMsg2.can_id  = frame_id_2;  // VOLTAGE ID: 002
  canMsg2.can_dlc = 8;

  canMsg3.can_id  = frame_id_3;  // Current, SOC, SOH and Capacity ID: 003
  canMsg3.can_dlc = 8;  
  
  canMsg4.can_id  = frame_id_4;  //  Balance Status ID: 004
  canMsg4.can_dlc = 4;

  while (!Serial);
  Serial.begin(115200);
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  
  Serial.println("Example: Write to CAN");
}

void float2byte(float x, byte *var, int resolution){
  *var = (int)(x*resolution) & 0xff;
  *(var+1) = (int)(x*resolution) >> 8 & 0xff;
}
float byte2float(byte *var, int resolution){
  //0x03F4 => 1012 => 10.12
  return ((float) *reinterpret_cast<int*>(var))/resolution;
}

void updateCANmessages(){
  
  can_counter=0;  
  for(int i = 0; i < (canMsg1.can_dlc/2); i++){ // TEMPERATURE
  float2byte(temp_list[i],tmsg,resolution);
  Serial.print(tmsg[1],HEX);
  Serial.println(tmsg[0],HEX);
  canMsg1.data[can_counter] = tmsg[1];   
  canMsg1.data[can_counter+1] = tmsg[0];
  can_counter = can_counter + 2;
  }
  can_counter=0; 
  for(int i = 0; i < (canMsg2.can_dlc/2); i++){ // VOLTAGE
  float2byte(vol_list[i],tmsg,resolution);
  Serial.print(tmsg[1],HEX);
  Serial.println(tmsg[0],HEX);
  canMsg2.data[can_counter] = tmsg[1];   
  canMsg2.data[can_counter+1] = tmsg[0];
  can_counter = can_counter + 2;
  }
  can_counter=0; 
  /*for(int i = 0; i < (canMsg3.can_dlc/2); i++){ // Current, SOC, SOH and Capacity
  float2byte(temp_list[i],tmsg,resolution);
  Serial.print(tmsg[1],HEX);
  Serial.println(tmsg[0],HEX);
  canMsg3.data[can_counter] = tmsg[1];   
  canMsg3.data[can_counter+1] = tmsg[0];
  can_counter = can_counter + 2;
  }
  can_counter=0; 
  */
  for(int i = 0; i<canMsg4.can_dlc; i++){ // Balance Status
  canMsg4.data[i] = balance_list[i];   
  }
}

void sendCANmessages(){
  mcp2515.sendMessage(&canMsg1);  // CAN Message for Temperatures
  mcp2515.sendMessage(&canMsg2);  // CAN Message for Voltages
  delay(100);
  mcp2515.sendMessage(&canMsg3);
  mcp2515.sendMessage(&canMsg4);
  delay(100);
}

void loop() {

  Serial.println("start");
  updateCANmessages();  
  sendCANmessages();
  
}
