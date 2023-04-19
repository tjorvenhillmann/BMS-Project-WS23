#include <Arduino.h>

// ********************** global definitions **********************
bool safe = false; 
int SOC = 0; // values from 0 to 100
int v_ref = 5000; // reference voltage in mV

// fault indications 
bool current_fault = false; 

// measurement data 
int temp_1 = 0; 
int temp_2 = 0; 
int temp_3 = 0; 
int temp_4 = 0; 
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
bool battery_switch = false; // battery disconnected 

// safety limits (read-only)
const int cutoff_temp_upper_limit = 60; // [°C]
const int cutoff_temp_lower_limit = -20; // [°C]
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
void checkCurrent_withACS712();
void checkVoltage();
void checkTemp();
void setFaultCondition();
void setSafeCondition();
void calculateSOC();

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

void checkCurrent_withACS712(){
  // read sensor
  int offset = 2500; // [mV], 0A at 2.5V
  int sensValue = analogRead(CURRENT_PIN); // read sensor value range 0-1024
  double sensVoltage = (sensValue/1024)*v_ref; // calculate voltage in mV
  double mAmp = ((sensVoltage - offset)/66.0)*1000.0; // [mA], sensor measures 66 mV/A
  
  current = mAmp; // [mA] convert double to int -> Rundet nicht richtig!!

 // check current limits 
  if(current >= positive_cutoff_current){
    // DISCONNECT! 
    battery_switch = false; 
    current_fault = true;
  }

  if(current <= negative_cutoff_current){
    // DISCONNECT! 
    battery_switch = false; 
    current_fault = true; 
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
  cell_1_V = analogRead(CELL_1_V_PIN); 
  cell_2_V = analogRead(CELL_2_V_PIN);
  cell_3_V = analogRead(CELL_3_V_PIN);
  cell_4_V = analogRead(CELL_4_V_PIN);
 
  //check for every value
  int cell_V[] = {cell_1_V, cell_2_V, cell_3_V, cell_4_V}; 
  for(i=0; i < sizeof(cell_V), i++){ 
    if(cell_V[i] >= cutoff_voltage_upper_limit){ //high voltage
      // DISCHARGE!!! --> Balancing Enable
    }else if(cell_V[i] <= cutoff_voltage_lower_limit){ //low voltage
      // CHARGE!!!
    }
  }
}

void checkTemperature(){
  // 4 temperature sensors, each value 2 byte
  // read sensors
  int temp_1_sensValue = analogRead(TEMP_1_PIN); 
  int temp_2_sensValue = analogRead(TEMP_2_PIN);
  int temp_3_sensValue = analogRead(TEMP_3_PIN);
  int temp_4_sensValue = analogRead(TEMP_4_PIN);
 

  //check for every value
  for(){ 
    if(){ //no feedback

    }else if(){ //high temperature

    }else if(){ //low temperature

    }else{
      
    }
  }
}

void setFaultCondition(){

}

void setSafeCondition(){

}

void calculateSOC(){

}


void loop() {
  checkCurrent();
  checkVoltage();
  checkTemp();

  // safety
  if(safe==1){
    setSafeCondition();
  }else{
    setFaultCondition();
  }

  calculateSOC();
}