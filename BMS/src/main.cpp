#include <Arduino.h>
#include <math.h>
#include <SPI.h>
#include <mcp2515.h>

// ********************** global definitions **********************
bool startingup = true;
bool error = false; 
unsigned long prev_time = 0; 
float prev_voltage = 0; 
float soc; // values from 0 to 100
float soh; 
float rul; 
const float balThreshold = 0.030; // threshold for balancing [V]
const float stopBalThreshold = 0.01; // threshold for stopping [V]
bool charging = false; // indicates charging status: charging = true, discharging = false
bool old_status = false; 
unsigned long charging_timer_offset = 0; 

// parameters for loop delay
unsigned long lastMeasurement = 0; 
const unsigned long measurementInterval = 2e3; // measure every 2 seconds [ms]

// calibration data 
float v_ref = 5.0; // reference voltage in V
float current_calibration_factor = 0.667; 
// double temp_sens_offset = 12; // offset for temp_sensor [C]

// status indications 
bool current_fault = false; 
int temp_fault = 0; // 1: high temp; -1: low temp
int voltage_fault[4] = {0, 0, 0, 0}; // 1: high voltage; -1: low voltage 
int voltage_status[4] = {0, 0, 0, 0}; // voltage within balancing threshold to cut-off
                                      // 1: high voltage; -1: low voltage 

// measurement data 
float temp_1; 
float temp_2; 
float temp_3; 
float temp_4; 
float cell_1_V; 
float cell_2_V; 
float cell_3_V; 
float cell_4_V; 
float current; 

// output data 
bool balance_status_1 = false; 
bool balance_status_2 = false;
bool balance_status_3 = false; 
bool balance_status_4 = false; 
bool battery_switch = false;

// safety limits (read-only)
const float cutoff_temp_upper_limit_charging = 45.0; // [°C]
const float cutoff_temp_lower_limit_charging = 0; // [°C]
const float cutoff_temp_upper_limit_discharging = 60.0; // [°C]
const float cutoff_temp_lower_limit_discharging = -20.0; // [°C]
const float cutoff_voltage_lower_limit = 2.5; // [V]
const float cutoff_voltage_upper_limit = 4.2; // [V]
const float nom_voltage = 3.6; // [V]
const float nom_capacity = 1500.0; // [mAh]
const float stopChargingCurrent = 0.1; // [A]
const float charging_cutoff_current = 0.75; // current maximum [A] 
const float discharging_cutoff_current = -10.0; // negative current maximum [A] 
const long charge_time = 150.0*60.0*1000.0; // maximum charging time [ms]

// global variables for limits
float cutoff_temp_upper_limit = cutoff_temp_upper_limit_discharging; 
float cutoff_temp_lower_limit = cutoff_temp_lower_limit_discharging; 

// pin definition (read-only)
// analouge
const unsigned int TEMP_1_PIN = A0;
const unsigned int TEMP_2_PIN = A1;
const unsigned int TEMP_3_PIN = A2;
const unsigned int TEMP_4_PIN = A3;

const unsigned int CELL_1_V_PIN = A4;
const unsigned int CELL_2_V_PIN = A5;
const unsigned int CELL_3_V_PIN = A6;
const unsigned int CELL_4_V_PIN = A7;

const unsigned int CURRENT_PIN = A8;

// digital
const unsigned int BALANCE_STATUS_1_PIN = 22; 
const unsigned int BALANCE_STATUS_2_PIN = 23; 
const unsigned int BALANCE_STATUS_3_PIN = 24; 
const unsigned int BALANCE_STATUS_4_PIN = 25; 
const unsigned int BATTERY_SWITCH_PIN = 26; 

// switches (currently unused)
const unsigned int SWITCH_0 = 30; 
const unsigned int SWITCH_1 = 32; 
const unsigned int SWITCH_2 = 34; 
const unsigned int SWITCH_3 = 36;

// CAN Variables & Libraries
const unsigned int CS_PIN = 49; // // CS PIN for MCP CAN Module
MCP2515 mcp2515(CS_PIN); 
struct can_frame canMsg1;
struct can_frame canMsg2;
struct can_frame canMsg3;
struct can_frame canMsg4;

byte frame_id_1 = 0x001;	// Temperature ID: 001
byte frame_id_2 = 0x002;	// VOLTAGE ID: 002
byte frame_id_3 = 0x003;	// Current, SOC, SOH ID: 003
byte frame_id_4 = 0x004;	//  Balance Error Status ID: 004

float temp_list[] = {temp_1, temp_2, temp_3,temp_4};
float vol_list[] = {cell_1_V, cell_2_V, cell_3_V, cell_4_V};
float current_soc_soh[] = {current, soc, soh};
bool balance_error_list[] = {balance_status_1, balance_status_2, balance_status_3, balance_status_4, error};

int resolution = 100;
int can_counter=0;
byte tmsg[2] = {0x00,0x00}; 

// **************************************************************

// ********************* functions ******************************
float adc2temp(int16_t adc);
// double readThermisterSE017();
void checkCurrent_withACS712();
void checkVoltage();
void checkTemp();
void controlBalancing(); 
void calculateStartSOC();
void battery_state();
void adjust_temp_limits();
void connectBattery();

// **************************************************************


void setup() {
  // put your setup code here, to run once: 
  // Pin configuration
  Serial.begin(9600); 
  // Analouge pins A0-A3 for temperature sensors
  pinMode(TEMP_1_PIN, INPUT); 
  pinMode(TEMP_2_PIN, INPUT); 
  pinMode(TEMP_3_PIN, INPUT); 
  pinMode(TEMP_4_PIN, INPUT); 
  // Analouge pins A4-A7 for voltage sensors
  pinMode(CELL_1_V_PIN, INPUT); 
  pinMode(CELL_2_V_PIN, INPUT); 
  pinMode(CELL_3_V_PIN, INPUT); 
  pinMode(CELL_4_V_PIN, INPUT); 
  // Analouge pin A8 for current sensor
  pinMode(CURRENT_PIN, INPUT); 
  // Digital pins D0-D3 for balancing
  pinMode(BALANCE_STATUS_1_PIN, OUTPUT);
  pinMode(BALANCE_STATUS_2_PIN, OUTPUT);
  pinMode(BALANCE_STATUS_3_PIN, OUTPUT);
  pinMode(BALANCE_STATUS_4_PIN, OUTPUT);
  pinMode(BATTERY_SWITCH_PIN, OUTPUT); 
  // switches  
  pinMode(SWITCH_0, OUTPUT); 
  pinMode(SWITCH_1, OUTPUT); 
  pinMode(SWITCH_2, OUTPUT); 
  pinMode(SWITCH_3, OUTPUT);

  // CAN MESSAGES
  while (!Serial);
  canMsg1.can_id  = frame_id_1;  // Temperature ID: 001
  canMsg1.can_dlc = 8;
  canMsg2.can_id  = frame_id_2;  // VOLTAGE ID: 002
  canMsg2.can_dlc = 8;
  canMsg3.can_id  = frame_id_3;  // Current, SOC, SOH ID: 003
  canMsg3.can_dlc = 6;  
  canMsg4.can_id  = frame_id_4;  //  Balance Error Status ID: 004
  canMsg4.can_dlc = 5;
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
}

float adc2temp(int16_t adc){
  float adc_f = (float)adc;
  float x4 = pow(adc_f, 4) * -2.864e-09;
  float x3 = pow(adc_f, 3) * 3.726e-06;
  float x2 = pow(adc_f, 2) * -0.00142;
  float x1 = adc_f * -0.01458;
  float x0 = 97.45;
  return x4 + x3 + x2 + x1 + x0;
}

// float readThermisterSE017(int RawADC){
//  float lnR, Temp; 
//  lnR = log(((10240000/RawADC) - 10000)); 
//  Temp = 1/(0.001129148 + (0.000234125 + (0.0000000876741 * lnR * lnR )) * lnR);
//  Temp = -1*(Temp - 273.15 - temp_sens_offset); 
//  return Temp; 
// }

void checkCurrent_withACS712(){
  // read sensor
  float offset = 2.500; // [V], 0A at 2.5V
  int sensValue = analogRead(CURRENT_PIN); // read sensor value range 0-1024
  double sensVoltage = (sensValue/1024.0)*v_ref; // calculate voltage in V
  float Amp = ((sensVoltage - offset)/0.066)*current_calibration_factor; // [A], sensor measures 66 mV/A
  
  current = Amp; // [A]
  current_soc_soh[0] = current; // Update for CAN

 // check current limits 
  if(current >= charging_cutoff_current){
    current_fault = true;
  }else if(current <= discharging_cutoff_current){
    current_fault = true; 
  }else{
    current_fault = false;
  }
}

void checkVoltage(){
  // 4 voltage sensors, each value 2 byte
  int i = 0; 

  // read sensors
  int cell_1_sensValue = analogRead(CELL_1_V_PIN); 
  int cell_2_sensValue = analogRead(CELL_2_V_PIN);
  int cell_3_sensValue = analogRead(CELL_3_V_PIN);
  int cell_4_sensValue = analogRead(CELL_4_V_PIN);

  // compute voltage [mV]
  cell_1_V = cell_1_sensValue * (v_ref/1023.0);
  cell_2_V = cell_2_sensValue * (v_ref/1023.0);
  cell_3_V = cell_3_sensValue * (v_ref/1023.0);
  cell_4_V = cell_4_sensValue * (v_ref/1023.0); 

  // Update for CAN-Message
  vol_list[0] = cell_1_V; 
  vol_list[1] = cell_2_V; 
  vol_list[2] = cell_3_V; 
  vol_list[3] = cell_4_V; 

 
  //check for every value
  float cell_V[] = {cell_1_V, cell_2_V, cell_3_V, cell_4_V}; 
  int j = sizeof(cell_V)/sizeof(cell_V[0]);

  for(i=0; i<j; i++){ 
    // Status Check
    if(cell_V[i] >= cutoff_voltage_upper_limit-balThreshold){ // on upper limit 
      voltage_status[i] = 1; 
    }else if(cell_V[i] <= cutoff_voltage_lower_limit+balThreshold){ // on lower limit 
      voltage_status[i] = -1; 
    }else{
      voltage_status[i] = 0; 
    }
    // Cut-off Check
    if(cell_V[i] >= cutoff_voltage_upper_limit){ // high voltage
      voltage_fault[i] = 1; 
      voltage_status[i] = 0; 
    }else if(cell_V[i] <= cutoff_voltage_lower_limit){ // low voltage
      voltage_fault[i] = -1; 
      voltage_status[i] = 0;  
    }else{
      voltage_fault[i] = 0; 
    }
  }
}

void checkTemp(){
  unsigned int i = 0; 
  // 4 temperature sensors, each value 2 byte
  // read sensors
  int temp_1_sensValue = analogRead(TEMP_1_PIN); 
  int temp_2_sensValue = analogRead(TEMP_2_PIN);
  int temp_3_sensValue = analogRead(TEMP_3_PIN);
  int temp_4_sensValue = analogRead(TEMP_4_PIN);
 
 // calculate temperature with raw value 
  temp_1 = adc2temp(temp_1_sensValue); 
  temp_2 = adc2temp(temp_2_sensValue);
  temp_3 = adc2temp(temp_3_sensValue); 
  temp_4 = adc2temp(temp_4_sensValue);

  //check for every value
  float temp_C[] = {temp_1, temp_2, temp_3, temp_4}; 
  for(i=0; i < (sizeof(temp_C)/sizeof(temp_C[0])); i++){ 
    if(temp_C[i] >= cutoff_temp_upper_limit){  //high temperature
      temp_fault = 1; 
    }else if(temp_C[i] <= cutoff_temp_lower_limit){ //low temperature
      temp_fault = -1; 
    }else{
      temp_fault = 0; 
    }
  } 

  temp_list[0] = temp_C[0];
  temp_list[1] = temp_C[1]; 
  temp_list[2] = temp_C[2];   
  temp_list[3] = temp_C[3];    
}

void controlBalancing(){
  int i = 0; 
  float cell_V[] = {cell_1_V, cell_2_V, cell_3_V, cell_4_V}; 
  int j = sizeof(cell_V)/sizeof(cell_V[0]);
  bool balance_status[4] = {balance_status_1, balance_status_2, balance_status_3, balance_status_4};
  float maxVol = cell_V[0]; // [V]
  float minVol = cell_V[0]; // [V]
  int maxVol_index = 1; 
  int minVol_index = 1; 
  float volDiff = 0; 

  // find maximum voltage 
  for(i=0; i<j; i++){
    if(cell_V[i] > maxVol){
      maxVol = cell_V[i];
      maxVol_index = i; 
      Serial.println("maxVol_index einlesen");
      Serial.println(maxVol_index);
    }
  }
  // find minimum voltage
  i = 0;  
  for(i=0; i<j; i++){
    if(cell_V[i] < minVol){
      minVol = cell_V[i];
      minVol_index = i; 
    }
  }

  // Balancing control based on cell voltage difference
  // calculate difference
  volDiff = maxVol - minVol; 
  // Serial.println(volDiff);
  // enable balancing of cell with highest voltage if necessary
  if(volDiff > balThreshold){
    balance_status[maxVol_index] = true;
    Serial.print("balance status max Vol: "); 
    Serial.print(balance_status[maxVol_index]);
    Serial.print("\n"); 
    Serial.println("0 entry!!!");
  }
  // disable balancing of cell with lowest voltage 
  if(balance_status[minVol_index] == true){
    balance_status[minVol_index] = false; 
    Serial.println("1 entry!!!");
  }
  // disable balancing of all cells, if balanced
  if(volDiff < stopBalThreshold){
    for(i=0; i<j; i++){
      balance_status[i] = false;
      Serial.println("2 entry!!!");
    }
  }

  // Balancing control based on cell voltages 
  for(i=0; i<j; i++){
    if(voltage_status[i] == 1){ // voltage on upper limit -> enable 
      balance_status[i] = true; 
      Serial.println("3 entry!!!");
    }
  
    if(voltage_status[i] == -1){ // voltage on lower limit -> disable
      balance_status[i] = false; 
      Serial.println("4 entry!!!");
    }
  }

  // set values to hardware
  balance_status_1 = balance_status[0];
  balance_status_2 = balance_status[1];
  balance_status_3 = balance_status[2];
  balance_status_4 = balance_status[3];

  if(balance_status[0]){
    digitalWrite(BALANCE_STATUS_1_PIN, HIGH);
  }else{
    digitalWrite(BALANCE_STATUS_1_PIN, LOW);
  }

   if(balance_status[1]){
    digitalWrite(BALANCE_STATUS_2_PIN, HIGH);
  }else{
    digitalWrite(BALANCE_STATUS_2_PIN, LOW);
  }

   if(balance_status[2]){
    digitalWrite(BALANCE_STATUS_3_PIN, HIGH);
  }else{
    digitalWrite(BALANCE_STATUS_3_PIN, LOW);
  }
  
  if(balance_status[3]){
    digitalWrite(BALANCE_STATUS_4_PIN, HIGH);
  }else{
    digitalWrite(BALANCE_STATUS_4_PIN, LOW);
  }

  // Set to CAN-Message
  balance_error_list[0] = balance_status[0];
  balance_error_list[1] = balance_status[1];
  balance_error_list[2] = balance_status[2];
  balance_error_list[3] = balance_status[3];

  Serial.print("Balance Control: ");
  Serial.print(balance_status_1);
  Serial.print("\t");
  Serial.print(balance_status_2);
  Serial.print("\t");
  Serial.print(balance_status_3);
  Serial.print("\t");
  Serial.print(balance_status_4);
  Serial.print("\n");
  Serial.print("Balance Control array: ");
  Serial.print(balance_status[0]);
  Serial.print("\t");
  Serial.print(balance_status[1]);
  Serial.print("\t");
  Serial.print(balance_status[2]);
  Serial.print("\t");
  Serial.print(balance_status[3]);
  Serial.print("\n");
  Serial.print("Balancing Triggers: ");
  Serial.print("minVol: ");
  Serial.print(minVol);
  Serial.print("\t");
  Serial.print("maxVol: ");
  Serial.print(maxVol);
  Serial.print("\t");
  Serial.print("Difference: ");
  Serial.print(volDiff); 
  Serial.print("\n");
  Serial.print(minVol_index);
  Serial.print("\t");
  Serial.print(maxVol_index);
  Serial.print("\n");
}

void calculateStartSOC(){
  float medium_vol = ((cell_1_V+cell_2_V+cell_3_V+cell_4_V)/4.0); // [V]
  soc = (medium_vol-cutoff_voltage_lower_limit)/(cutoff_voltage_upper_limit-cutoff_voltage_lower_limit)*100.0;

  if(soc > 100.0){
    soc = 100; 
  }

  if(soc < 0){
    soc = 0; 
  }
}

void battery_state(){
  unsigned long current_time = millis(); 
  float elapsed_time = (current_time-prev_time)/1000.0; 
  float charge_change = -current * elapsed_time/3600.0; // [As]
  float medium_vol = ((cell_1_V+cell_2_V+cell_3_V+cell_4_V)/4.0); // [V]

  // SOC
  float new_soc = soc - 100 * charge_change/(nom_capacity/1000.0); 
  soc = max(0.0, min(100.0, new_soc)); 
  current_soc_soh[1] = soc; // Update for CAN

  // SOH 
  float new_soh = 100 * (medium_vol/cutoff_voltage_upper_limit) * (soc/100); 
  soh = max(0.0, min(100.0, new_soh)); 
  current_soc_soh[2] = soh; // Update for CAN
  // RUL
  float new_rul = 100 * (1- (soc/100)) * (soh/100); 
  rul = max(0.0, min(100.0, new_rul)); 

  prev_time = current_time; 
  prev_voltage = medium_vol;
}

void adjust_temp_limits(){
  // change temperature limtis depending on charging/discharging 
  if(charging == true){
    cutoff_temp_upper_limit = cutoff_temp_upper_limit_charging;
    cutoff_temp_lower_limit = cutoff_temp_lower_limit_charging;
  }else{
    cutoff_temp_upper_limit = cutoff_temp_upper_limit_discharging;
    cutoff_temp_lower_limit = cutoff_temp_lower_limit_discharging;
  }
}

void connectBattery(){ // to ensure, that only connects when safe
  if(error){
    battery_switch = false; 
    digitalWrite(BATTERY_SWITCH_PIN, LOW); 
  }else{
    battery_switch = true; 
    digitalWrite(BATTERY_SWITCH_PIN, HIGH); 
  }
  Serial.print("Battery Switch");
  Serial.print(battery_switch); 
  Serial.print("\n");
}


// CAN FUNCTIONS
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
  // Serial.print(tmsg[1],HEX);
  // Serial.println(tmsg[0],HEX);
  canMsg1.data[can_counter] = tmsg[1];   
  canMsg1.data[can_counter+1] = tmsg[0];
  can_counter = can_counter + 2;
  }
  can_counter=0; 
  for(int i = 0; i < (canMsg2.can_dlc/2); i++){ // VOLTAGE
  float2byte(vol_list[i],tmsg,resolution);
  // Serial.print(tmsg[1],HEX);
  // Serial.println(tmsg[0],HEX);
  canMsg2.data[can_counter] = tmsg[1];   
  canMsg2.data[can_counter+1] = tmsg[0];
  can_counter = can_counter + 2;
  }
  can_counter=0; 
  for(int i = 0; i < (canMsg3.can_dlc/2); i++){ // Current, SOC, SOH
  float2byte(current_soc_soh[i],tmsg,resolution);
  // Serial.print(tmsg[1],HEX);
  // Serial.println(tmsg[0],HEX);
  canMsg3.data[can_counter] = tmsg[1];   
  canMsg3.data[can_counter+1] = tmsg[0];
  can_counter = can_counter + 2;
  }
  can_counter=0; 
  
  for(int i = 0; i<canMsg4.can_dlc; i++){ // Balance Error Status
    if(balance_error_list[i] == false){
      canMsg4.data[i] = 0;   
    }
    else if (balance_error_list[i] == true){
      canMsg4.data[i] = 1;  
    }
  }
}

void sendCANmessages(){
  mcp2515.sendMessage(&canMsg1);  // CAN Message for Temperatures
  delay(100); 
  mcp2515.sendMessage(&canMsg2);  // CAN Message for Voltages
  delay(100); 
  mcp2515.sendMessage(&canMsg3);  // CAN Message for Current SOC SOH
  delay(100); 
  mcp2515.sendMessage(&canMsg4);  // CAN Message for Balance Error Status
  // delay(100);
}





void loop() {

  while((millis()-lastMeasurement) < measurementInterval){
    // delay - do nothing 
  }
  lastMeasurement = millis();

  checkCurrent_withACS712();
  checkVoltage();
  checkTemp();
  controlBalancing();
  battery_state();

  // measure start-soc -> performed only in first loop
  if(startingup){
    calculateStartSOC();
  }

  // charging control 
  old_status = charging; 
  if(current > 0){
    charging = true;
    if(old_status == false){ // wasn't charging before -> just started, set offset for timer
      charging_timer_offset = millis();
    }
  }else{
    charging = false;
  }
  adjust_temp_limits(); 
  // disconnect battery from charging
  if(charging){
    if(millis() > charging_timer_offset + charge_time){ // timeout
      battery_switch = false; 
    }else if(current < stopChargingCurrent){ // charging current 
      battery_switch = false; 
    }
  }

  // error report
  if(current_fault || voltage_fault[0] != 0 || voltage_fault[1] != 0
                    || voltage_fault[2] != 0 || voltage_fault[3] != 0 || temp_fault != 0){
    error = true;
    battery_switch = false;
    digitalWrite(BATTERY_SWITCH_PIN, LOW); 
  }else{
    error = false; 
  }
  balance_error_list[4] = error;

  // Connect battery if everything okay
  connectBattery(); 
  Serial.print("Battery Switch: "); 
  Serial.print(battery_switch); 
  Serial.print("\n"); 

  startingup = false;

 
  // SOC, SOH, Error
  Serial.print("SOC: "); 
  Serial.print(soc); 
  Serial.print("\t");
  Serial.print("SOH: "); 
  Serial.print(soh); 
  Serial.print("\t");
  Serial.print("Current: "); 
  Serial.print(current); 
  Serial.print("\t");
  Serial.print("Error: "); 
  Serial.print(error); 
  Serial.print("\n");
  // Cell voltages 
  Serial.print("Voltage: \t");
  Serial.print(cell_1_V);
  Serial.print("\t");
  Serial.print(cell_2_V);
  Serial.print("\t");
  Serial.print(cell_3_V);
  Serial.print("\t");
  Serial.print(cell_4_V);
  Serial.print("\n");

  // Cell temperatures
  Serial.print("Temp: \t \t");
  Serial.print(temp_1);
  Serial.print("\t");
  Serial.print(temp_2);
  Serial.print("\t");
  Serial.print(temp_3);
  Serial.print("\t");
  Serial.print(temp_4);
  Serial.print("\n");

  // Balance status
  Serial.print("Balancing: \t");
  Serial.print(balance_status_1);
  Serial.print("\t");
  Serial.print(balance_status_2);
  Serial.print("\t");
  Serial.print(balance_status_3);
  Serial.print("\t");
  Serial.print(balance_status_4);
  Serial.print("\n");
  Serial.print("------------------------------------ \n");
  
  // Update and Send CAN Messages
  updateCANmessages();  
  sendCANmessages();  
}