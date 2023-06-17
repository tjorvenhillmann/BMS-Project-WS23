#include <SPI.h>
#include <mcp2515.h>
#include <Arduino.h>

// Data struct for diffrent can frames
struct Voltages
{
  float cell_V1 = 0;
  float cell_V2 = 0;
  float cell_V3 = 0;
  float cell_V4 = 0; 
  float pack_voltage = 0;
};

struct Temps
{
  float temp1 = 0;
  float temp2 = 0;
  float temp3 = 0;
  float temp4 = 0;
};

struct CurSocSohCap
{
  float current = 0.0;
  uint16_t soc = 100;
  uint16_t soh = 99; // %
  uint16_t cap = 1500; // mAh
};

struct Status
{
  bool balance_status1 = 0;
  bool balance_status2 = 0;
  bool balance_status3 = 0;
  bool balance_status4 = 0;
};

// Additional variables
byte balancer_status = 0;
bool cloud_status = 0;

// Can receice frame 
struct can_frame canMsg;

// Response byte array used for converting to float 
byte msg[2] = {0,0};

// Create instance of CAN module
MCP2515 mcp2515(15);

// Create objects of frame structs 
Temps temps;
Voltages voltages;
CurSocSohCap curSocSohCap;
Status balance_states;

float byte2float(__u8 * var, int resolution){
  // Extract to bytes from adress
  // Dicuss wether we send MSB or LSB ?????
  msg[1] = *(var+1);
  msg[0] = *(var);
  return ((float) *reinterpret_cast<int*>(msg))/resolution;
}

void setupCAN()
{
  // All necessary steps and settimgs for init CAN module
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  Serial.println("------- CAN Read ----------");
  //Serial.println("ID  DLC   DATA");
}

void printCAN_Frames(){
  // Just a bunch of serial prints for debugging purposes
  if(canMsg.can_id == 1){
    Serial.println("New Temperature values received:");
    Serial.print(temps.temp1);
    Serial.print("    ");
    Serial.print(temps.temp2);
    Serial.print("    ");
    Serial.print(temps.temp3);
    Serial.print("    ");
    Serial.println(temps.temp4);
  }
  if(canMsg.can_id == 2){
    Serial.println("New Voltage values received:");
    Serial.print(voltages.cell_V1);
    Serial.print("    ");
    Serial.print(voltages.cell_V2);
    Serial.print("    ");
    Serial.print(voltages.cell_V3);
    Serial.print("    ");
    Serial.println(voltages.cell_V4);
  }
  if(canMsg.can_id == 3){
    Serial.println("New Cur, SOC, SOH and Cap values received:");
    Serial.print(curSocSohCap.current);
    Serial.print("    ");
    Serial.print(curSocSohCap.soc);
    Serial.print("    ");
    Serial.print(curSocSohCap.soc);
    Serial.print("    ");
    Serial.println(curSocSohCap.cap);
  }
  if(canMsg.can_id == 4){
  }
}

void decodeCAN_Message(){
  // Decode and store new message into structs for each frame 
  if(canMsg.can_id == 1){
    // Get all temp values from can_frame
    Serial.println(byte2float(&canMsg.data[0],100));
    temps.temp1 = byte2float(&canMsg.data[0],100);
    temps.temp2 = byte2float(&canMsg.data[2],100);
    temps.temp3 = byte2float(&canMsg.data[4],100);
    temps.temp4 = byte2float(&canMsg.data[6],100);
  }
  if(canMsg.can_id == 2){
    // Get all voltage values from can_frame and calculate pack voltage
    voltages.cell_V1 = byte2float(&canMsg.data[0],1000);
    voltages.cell_V2 = byte2float(&canMsg.data[2],1000);
    voltages.cell_V3 = byte2float(&canMsg.data[4],1000);
    voltages.cell_V4 = byte2float(&canMsg.data[6],1000);
  }
  if(canMsg.can_id == 3){
    // Get current, capacity, SOC and SOH values from can_frame
    curSocSohCap.current = byte2float(&canMsg.data[0],1000);
    curSocSohCap.soc = byte2float(&canMsg.data[2],10000);
    curSocSohCap.soh = byte2float(&canMsg.data[4],10000);
    curSocSohCap.cap = byte2float(&canMsg.data[6],10000);
  }
  if(canMsg.can_id == 4){
  }
}

void readCAN_Message()
{
  // When new message received print out message
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.println("New Message received");
    Serial.println("ID  DLC");
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print("    ");  
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.println(" ");
    // For printing data as raw byte values
    // for(int i = 0; i < canMsg.can_dlc;i++){
    //     Serial.print(canMsg.data[i],HEX);
    //     Serial.print(" ");
    //     Serial.println(canMsg.data[i],BIN);
    // }

    // Decode and store new message into structs for each frame 
    decodeCAN_Message();
    printCAN_Frames();
  }
}


void setup() {
  // Start the serial monitor with desired baudrate
  Serial.begin(115200);

  // Initialize the CAN module
  setupCAN();
}

void loop() {
  // Just read and display new messages until eternity for now
  readCAN_Message();
  delay(100);
}
