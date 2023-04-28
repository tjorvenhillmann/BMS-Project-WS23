#include <Arduino.h>
#include <math.h>

// ********************** global definitions **********************
// bool safe = false; 
unsigned long prev_time = 0; 
float prev_voltage = 0; 
float soc = 0; // values from 0 to 100
float soh = 0; 
float rul = 0; 
int balThreshold = 30; // threshold for balancing [mV]
int stopBalThreshold = 5; // threshold for stopping [mV]

// calibration data 
int v_ref = 5000; // reference voltage in mV
double temp_sens_offset = 12; // offset for temp_sensor [C]

// battery parameters
float nom_capacity = 1000; 

// status indications 
bool current_fault = false; 
int temp_fault = 0; // 1: high temp; -1: low temp
int voltage_fault[4] = {0, 0, 0, 0}; // 1: high voltage; -1: low voltage 
int voltage_status[4] = {0, 0, 0, 0}; // voltage within balancing threshold to cut-off
                                      // 1: high voltage; -1: low voltage 

// measurement data 
float temp_1 = 0; 
float temp_2 = 0; 
float temp_3 = 0; 
float temp_4 = 0; 
int cell_1_V = 0; 
int cell_2_V = 0; 
int cell_3_V = 0; 
int cell_4_V = 0; 
int current = 0; 

// output data 
bool balance_status_1 = false; 
bool balance_status_2 = false;
bool balance_status_3 = false; 
bool balance_status_4 = false; 
bool battery_switch = false;

// safety limits (read-only)
const float cutoff_temp_upper_limit = 60; // [°C]
const float cutoff_temp_lower_limit = -20; // [°C]
const int cutoff_voltage_lower_limit = 3200; // [mV]
const int cutoff_voltage_upper_limit = 4200; // [mV]
const int positive_cutoff_current = 30000; // current maximum 30A [mA] 
const int negative_cutoff_current = -30000; // negative current maximum -30A [mA] 

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

// **************************************************************

// ********************* functions ******************************
double readThermisterSE017();
void checkCurrent_withACS712();
void checkVoltage();
void checkTemp();
void controlBalancing(); 
// void setFaultCondition();
// void setSafeCondition();
void battery_state();

// **************************************************************


void setup() {
  // put your setup code here, to run once: 
  // Pin configuration
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
}

float readThermisterSE017(int RawADC){
  float lnR, Temp; 
  lnR = log(((10240000/RawADC) - 10000)); 
  Temp = 1/(0.001129148 + (0.000234125 + (0.0000000876741 * lnR * lnR )) * lnR);
  Temp = -1*(Temp - 273.15 - temp_sens_offset); 
  return Temp; 
}

void checkCurrent_withACS712(){
  // read sensor
  int offset = 2500; // [mV], 0A at 2.5V
  int sensValue = analogRead(CURRENT_PIN); // read sensor value range 0-1024
  double sensVoltage = (sensValue/1024)*v_ref; // calculate voltage in mV
  double mAmp = ((sensVoltage - offset)/66.0)*1000.0; // [mA], sensor measures 66 mV/A
  
  current = mAmp; // [mA] convert double to int -> Rundet nicht richtig!!

 // check current limits 
  if(current >= positive_cutoff_current){
    battery_switch = false; 
    current_fault = true;
  }else if(current <= negative_cutoff_current){
    battery_switch = false; 
    current_fault = true; 
  }else{
    current_fault = false;
  }

  /*
  // Display
  Serial.print("sensor value = ");
  Serial.print(sensValue);
  Serial.print("\t sensor voltage in mV = ");
  Serial.print(sensVoltage);
  Serial.print("\t mAmp = ");
  Serial.print(mAmp); 
  Serial.print("\t current in mA = ");
  Serial.print(current); 
  Serial.print("\n");
  */
 
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

 
  //check for every value
  int cell_V[] = {cell_1_V, cell_2_V, cell_3_V, cell_4_V}; 
  int j = sizeof(cell_V)/sizeof(cell_V[0]);

  for(i=0; i<j; i++){ 
    // Status Check
    if(cell_V[i] >= cutoff_voltage_upper_limit-balThreshold){ // on upper limit 
      voltage_status[i] = 1; 
    }else if(cell_V[i] >= cutoff_voltage_lower_limit+balThreshold){ // on lower limit 
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

void checkTemperature(){
  int i = 0; 
  // 4 temperature sensors, each value 2 byte
  // read sensors
  int temp_1_sensValue = analogRead(TEMP_1_PIN); 
  int temp_2_sensValue = analogRead(TEMP_2_PIN);
  int temp_3_sensValue = analogRead(TEMP_3_PIN);
  int temp_4_sensValue = analogRead(TEMP_4_PIN);
 
 // calculate temperature with raw value 
  temp_1 = readThermisterSE017(temp_1_sensValue); 
  temp_2 = readThermisterSE017(temp_2_sensValue);
  temp_3 = readThermisterSE017(temp_3_sensValue); 
  temp_4 = readThermisterSE017(temp_4_sensValue);

  //check for every value
  float temp_C[] = {temp_1, temp_2, temp_3, temp_4}; 
  for(i=0; i < (sizeof(temp_C)/sizeof(temp_C[0])); i++){ 
    //high temperature
    if(temp_C[i] >= cutoff_temp_upper_limit){ 
      // DISCONNECT !!! 
      battery_switch = false; 
      temp_fault = 1; 
    }else if(temp_C[i] <= cutoff_temp_lower_limit){ //low temperature
      // DISCONNECT !!!
      battery_switch = false; 
      temp_fault = -1; 
    }else{
      temp_fault = 0; 
    }
  } 

  /*
  // Display
  Serial.print("temp 1 value = ");
  Serial.print(temp_1);
  Serial.print("\t temp 2 value = ");
  Serial.print(temp_2);
  Serial.print("\t temp 3 value = ");
  Serial.print(temp_3);
  Serial.print("\t temp 4 value = ");
  Serial.print(temp_4);
  */
}

void controlBalancing(){
  int i = 0; 
  int cell_V[] = {cell_1_V, cell_2_V, cell_3_V, cell_4_V}; 
  int j = sizeof(cell_V)/sizeof(cell_V[0]);
  bool balance_status[4] = {balance_status_1, balance_status_2, balance_status_3, balance_status_4};
  int maxVol = cell_V[0]; // [mV]
  int minVol = cell_V[0]; // [mV]
  int maxVol_index = 1; 
  int minVol_index = 1; 
  int volDiff = 0; 

  // find maximum voltage 
  for(i=0; i<j; i++){
    if(cell_V[i] > maxVol){
      maxVol = cell_V[i];
      maxVol_index = i; 
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
  // enable balancing of cell with highest voltage if necessary
  if(volDiff > balThreshold){
    balance_status[maxVol_index] = true;
  }
  // disable balancing of all cells, if balanced
  if(volDiff < stopBalThreshold){
    for(i=0; i<j; i++){
      balance_status[i] = false;
    }
  }

  // Balancing control based on cell voltages 
  for(i=0; i<j; i++){
    if(voltage_status[i] = 1){ // voltage on upper limit -> enable 
      balance_status[i] = true; 
    }

    if(voltage_status[i] = -1){ // voltage on lower limit -> disable
      balance_status[i] = false; 
    }
  }
}

void setFaultCondition(){

}

void setSafeCondition(){

}

void battery_state(){
  unsigned long current_time = millis(); 
  float elapsed_time = (current_time-prev_time)/1000.0; 
  float charge_change = (current/1000.0) * elapsed_time/3600.0; // [As]
  float medium_vol = ((cell_1_V+cell_2_V+cell_3_V+cell_4_V)/4.0); // [mV]

  // SOC
  float new_soc = soc - 100 * charge_change/(nom_capacity/1000.0); 
  soc = max(0.0, min(100.0, new_soc)); 

  // SOH 
  float new_soh = 100 * (medium_vol/cutoff_voltage_upper_limit)/1000.0 * (soc/100); 
  soh = max(0.0, min(100.0, new_soh)); 

  // RUL
  float new_rul = 100 * (1- (soc/100)) * (soh/100); 
  rul = max(0.0, min(100.0, new_rul)); 

  prev_time = current_time; 
  prev_voltage = medium_vol;
}


void loop() {
  checkCurrent_withACS712();
  checkVoltage();
  checkTemp();
  controlBalancing();
  battery_state();

  /* // safety
  if(safe==1){
    setSafeCondition();
  }else{
    setFaultCondition();
  } */
}