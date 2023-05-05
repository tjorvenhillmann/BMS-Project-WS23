#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;
MCP2515 mcp2515(10); // CS PIN from MCP CAN Module

int resolution =100;
byte frame_id_1 = 0x001;  // temperature ID
byte frame_id_2 = 0x002;  // Voltage ID
byte frame_id_3 = 0x003;  // Current SOH
byte frame_id_4 = 0x004;  // Balance Status
byte hexmsg[2] = {0,0};

float temp_list[4];
float vol_list[4];
float current_soc_soh_list[4];
bool balance_list[4];



void setup() {
  Serial.begin(115200);
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  
  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA");
}
float byte2float(byte * var, int resolution){
  return ((float) *reinterpret_cast<int*>(var))/resolution;
}

void getCANmessage(){
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id == frame_id_1)
    {
      for (int i = 0; i<canMsg.can_dlc; i+=2)  {  // print the data
      Serial.print("Temp: ");
      hexmsg[1] = canMsg.data[i];
      hexmsg[0] = canMsg.data[i+1];
      temp_list[i] = byte2float(hexmsg, 100);
      Serial.println(temp_list[i]);
      }  
    }
    else if (canMsg.can_id == frame_id_2)
    {
      for (int i = 0; i<canMsg.can_dlc; i+=2)  {  // print the data
      Serial.print("Voltage: ");
      hexmsg[1] = canMsg.data[i];
      hexmsg[0] = canMsg.data[i+1];
      vol_list[i] = byte2float(hexmsg, 100);
      Serial.println(vol_list[i]);
      }  
    }
    else if (canMsg.can_id == frame_id_3)
    {
      Serial.println("Current & SOH & SOC & CAPACITY: ");
      for (int i = 0; i<canMsg.can_dlc; i+=2)  {  // print the data
        hexmsg[1] = canMsg.data[i];
        hexmsg[0] = canMsg.data[i+1];
        current_soc_soh_list[i] = byte2float(hexmsg, 100);
        Serial.println( current_soc_soh_list[i]);
      }      
    }
    else if (canMsg.can_id == frame_id_4)
    {
      for (int i = 0; i<canMsg.can_dlc; i+=1)  {  // print the data
      Serial.print("Balance Status: ");
      if(canMsg.data[i] == 1){
        balance_list[i] = true;
      }
      else if(canMsg.data[i] == 0){
        balance_list[i] = false;
      }
      Serial.println(balance_list[i]);
      }      
    }
    
    delay(50);
  }
}


void loop() {

getCANmessage();

}
