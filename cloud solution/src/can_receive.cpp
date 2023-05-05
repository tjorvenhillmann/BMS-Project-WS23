#include <SPI.h>
#include <mcp2515.h>
#include <Arduino.h>

float temp_1 = 0;
float temp_2 = 0;
float temp_3 = 0;
float temp_4 = 0;

float cell_1_v = 0;
float cell_2_v = 0;
float cell_3_v = 0;
float cell_4_v = 0;

float current = 0;
float SOC = 0;
float SOH = 0;
float capacity = 0;

byte balancer_status = 0;

bool balanc_status_cell_1 = 0;
bool balanc_status_cell_2 = 0;
bool balanc_status_cell_3 = 0;
bool balanc_status_cell_4 = 0;

bool cloud_status = 0;

struct can_frame canMsg;
struct can_frame frames[4];

byte msg[2];

MCP2515 mcp2515(10);


void setup() {
  Serial.begin(115200);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA");
}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print(" "); 
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.println(" ");
    for(int i = 0; i < canMsg.can_dlc;i++){
        Serial.print(canMsg.data[i],HEX);
        Serial.print(" ");
        Serial.println(canMsg.data[i],BIN);
    }
    Serial.println("New Message received");     
  }
}

float byte2float(byte * var, int resolution){
  return ((float) *reinterpret_cast<int*>(var))/resolution;
}

