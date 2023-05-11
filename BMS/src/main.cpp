#include <Arduino.h>
#include <math.h>

// ********************** global definitions **********************
bool error = true; 
unsigned long prev_time = 0; 
float prev_voltage = 0; 
float soc = 0; // values from 0 to 100
float soh = 0; 
float rul = 0; 
float balThreshold = 0.030; // threshold for balancing [V]
float stopBalThreshold = 0.005; // threshold for stopping [V]
bool charging = false; // indicates charging status: charging = true, discharging = false
bool old_status = false; 
unsigned long charging_timer_offset = 0; 

// parameters for loop delay
unsigned long lastMeasurement = 0; 
unsigned long measurementInterval = 5; // measure every 5 seconds

// calibration data 
float v_ref = 5.0; // reference voltage in V
// double temp_sens_offset = 12; // offset for temp_sensor [C]

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
float cell_1_V = 0; 
float cell_2_V = 0; 
float cell_3_V = 0; 
float cell_4_V = 0; 
float current = 0; 

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
const float discharging_cutoff_current = -18.0; // negative current maximum [A] 
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

// **************************************************************

// ********************* functions ******************************
float adc2temp(int16_t adc);
// double readThermisterSE017();
void checkCurrent_withACS712();
void checkVoltage();
void checkTemp();
void controlBalancing(); 
void battery_state();
void adjust_temp_limits();
void connectBattery();

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
  int offset = 2500; // [mV], 0A at 2.5V
  int sensValue = analogRead(CURRENT_PIN); // read sensor value range 0-1024
  double sensVoltage = (sensValue/1024)*v_ref; // calculate voltage in mV
  float Amp = ((sensVoltage - offset)/66.0); // [A], sensor measures 66 mV/A
  
  current = Amp; // [A]

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

 
  //check for every value
  float cell_V[] = {cell_1_V, cell_2_V, cell_3_V, cell_4_V}; 
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
    if(voltage_status[i] == 1){ // voltage on upper limit -> enable 
      balance_status[i] = true; 
    }
  
    if(voltage_status[i] == -1){ // voltage on lower limit -> disable
      balance_status[i] = false; 
    }
  }
}

void battery_state(){
  unsigned long current_time = millis(); 
  float elapsed_time = (current_time-prev_time)/1000.0; 
  float charge_change = current * elapsed_time/3600.0; // [As]
  float medium_vol = ((cell_1_V+cell_2_V+cell_3_V+cell_4_V)/4.0); // [V]

  // SOC
  float new_soc = soc - 100 * charge_change/(nom_capacity/1000.0); 
  soc = max(0.0, min(100.0, new_soc)); 

  // SOH 
  float new_soh = 100 * (medium_vol/cutoff_voltage_upper_limit) * (soc/100); 
  soh = max(0.0, min(100.0, new_soh)); 

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
  }else{
    battery_switch = true; 
  }
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
  }else{
    error = false; 
  }
}