#include <Arduino.h>
#include <mcp2515.h>

#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// ----------------------------------------------INFO----------------------------------------------- //
// This file contains a method for creating an InfluxDB client object that allows you to upload data // 
// All necessary parameters are defined in this file to remove messy code from the main file         //
// We also establish the wifi connection in here to get the right time from the network              //
// ----------------------------------------------INFO----------------------------------------------- //


// WiFi AP SSID
#define WIFI_SSID "Pixel7"
// WiFi password
#define WIFI_PASSWORD "423fa176fba"
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
// InfluxDB 1.8+  (v2 compatibility API) server url, e.g. http://192.168.1.48:8086
#define INFLUXDB_URL "https://eu-central-1-1.aws.cloud2.influxdata.com"
// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
// InfluxDB 1.8+ (v2 compatibility API) use form user:password, eg. admin:adminpass
#define INFLUXDB_TOKEN "3MKqIhDWG-MiVgwLJMo2Cd9VrT4JfTof0_LKKrd6xauNHPoWkrudm6P7QaaR1t5MJ0B_gJw409xt1yJLjJMR1w=="
// InfluxDB v2 organization name or id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
// InfluxDB 1.8+ (v2 compatibility API) use any non empty string
#define INFLUXDB_ORG "Cloud-Solution/81721c757fe2e372"
// InfluxDB v2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
// InfluxDB 1.8+ (v2 compatibility API) use database name
#define INFLUXDB_BUCKET "ESP32"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
//  Central Europe: Berlin  	CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3" 
  
// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Can receice frame 
struct can_frame canMsg;

// Response byte array used for converting to float 
byte msg[2] = {0,0};
bool newMsgFlag = false;

// Create instance of CAN module
MCP2515 mcp2515(15);

// Real data points
Point bms_voltage("bms_voltage");
Point bms_temps("bms_temps");
Point bms_stats("bms_stats");
Point bms_balStatus("bms_balStatus");

struct Voltages
{
  float cell_V1 = 0.0;
  float cell_V2 = 0.0;
  float cell_V3 = 0.0;
  float cell_V4 = 0.0; 
};

struct Temps
{
  float temp1 = 0.0;
  float temp2 = 0.0;
  float temp3 = 0.0;
  float temp4 = 0.0;
};

struct CurSoCSoHCap
{
  float current = 0.0;
  float soc = 0.0;
  float soh = 0.0; // %
  float cap = 1500.0; // mAh
};

struct Status
{
  bool balance_status1 = 0;
  bool balance_status2 = 0;
  bool balance_status3 = 0;
  bool balance_status4 = 0;
};

// Create struct objects
Voltages voltages;
Temps temps;
CurSoCSoHCap curSocSohCap;
Status status;

float byte2float(__u8 * var, int resolution){
     // Extract to bytes from adress
    // Dicuss wether we send MSB or LSB ?????
    msg[0] = *(var+1);
    msg[1] = *(var);
    // Serial.print(msg[1],HEX);
    // Serial.println(msg[0],HEX);
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
    //Serial.println(byte2float(&canMsg.data[0],100));
    temps.temp1 = byte2float(&canMsg.data[0],100);
    temps.temp2 = byte2float(&canMsg.data[2],100);
    temps.temp3 = byte2float(&canMsg.data[4],100);
    temps.temp4 = byte2float(&canMsg.data[6],100);
  }
  if(canMsg.can_id == 2){
    // Get all voltage values from can_frame and calculate pack voltage
    voltages.cell_V1 = byte2float(&canMsg.data[0],100);
    voltages.cell_V2 = byte2float(&canMsg.data[2],100);
    voltages.cell_V3 = byte2float(&canMsg.data[4],100);
    voltages.cell_V4 = byte2float(&canMsg.data[6],100);
  }
  if(canMsg.can_id == 3){
    // Get current, capacity, SOC and SOH values from can_frame
    curSocSohCap.current = byte2float(&canMsg.data[0],100);
    curSocSohCap.soc = byte2float(&canMsg.data[2],100);
    curSocSohCap.soh = byte2float(&canMsg.data[4],100);
    //curSocSohCap.cap = byte2float(&canMsg.data[6],100);
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
    newMsgFlag = true;
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
  //Serial.println("No message received!");
}

void set_random_voltages() {
  int v1 = 0;
  int v2 = 0;
  int v3 = 0;
  int v4 = 0;
  v1 = random(300,420);
  v2 = random(300,420);
  v3 = random(300,420);
  v4 = random(300,420);
  voltages.cell_V1 = v1/100.0;
  voltages.cell_V2 = v2/100.0;
  voltages.cell_V3 = v3/100.0;
  voltages.cell_V4 = v4/100.0;
  Serial.println(voltages.cell_V1);
  Serial.println(voltages.cell_V2);
  Serial.println(voltages.cell_V3);
  Serial.println(voltages.cell_V4);
}

void set_random_temps(){
  int t1 = 0;
  int t2 = 0;
  int t3 = 0;
  int t4 = 0;
  t1 = random(1500,5000);
  t2 = random(1500,5000);
  t3 = random(1500,5000);
  t4 = random(1500,5000);
  temps.temp1 = t1/100.0;
  temps.temp2 = t2/100.0;
  temps.temp3 = t3/100.0;
  temps.temp4 = t4/100.0;
  Serial.println(temps.temp1);
  Serial.println(temps.temp2);
  Serial.println(temps.temp3);
  Serial.println(temps.temp4);
}


void setup() {
    Serial.begin(115200);

    // Setup wifi
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to wifi");
    while (wifiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();

    // Alternatively, set insecure connection to skip server certificate validation 
    //client.setInsecure();

    // Accurate time is necessary for certificate validation and writing in batches
    // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
    // Syncing progress and the time will be printed to Serial.
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

    // Check server connection
    if (client.validateConnection()) {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(client.getServerUrl());
    } 
    else {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(client.getLastErrorMessage());
    }

    // Setup CAN controller
    setupCAN();
}


void loop() {
    // Section for getting new CAN data 
    readCAN_Message();

    if (newMsgFlag)
    {
        // Store measured value into point
        bms_voltage.clearFields();
        bms_temps.clearFields();
        bms_stats.clearFields();
        bms_balStatus.clearFields();
        // Report RSSI of currently connected network
        // set_random_voltages();
        // set_random_temps();
        static float pack_voltage = 0.0;
        pack_voltage = voltages.cell_V1 + voltages.cell_V2 + voltages.cell_V3 + voltages.cell_V4;
        bms_voltage.addField("cell_V1", voltages.cell_V1);
        bms_voltage.addField("cell_V2", voltages.cell_V2);
        bms_voltage.addField("cell_V3", voltages.cell_V3);
        bms_voltage.addField("cell_V4", voltages.cell_V4);
        bms_voltage.addField("pack_voltage", pack_voltage);
        bms_temps.addField("Temp_Cell1", temps.temp1);
        bms_temps.addField("Temp_Cell2", temps.temp2);
        bms_temps.addField("Temp_Cell3", temps.temp3);
        bms_temps.addField("Temp_Cell4", temps.temp4);
        bms_stats.addField("SoC", curSocSohCap.soc);
        bms_stats.addField("SoH", curSocSohCap.soh);
        bms_stats.addField("Current", curSocSohCap.current);
        bms_stats.addField("Capacity", curSocSohCap.cap);
        bms_balStatus.addField("balance1", status.balance_status1);
        bms_balStatus.addField("balance2", status.balance_status1);
        bms_balStatus.addField("balance3", status.balance_status1);
        bms_balStatus.addField("balance4", status.balance_status1);

        // Print what are we exactly writing
        // Serial.print("Writing: ");
        // Serial.println(client.pointToLineProtocol(bms_voltage));
        // If no Wifi signal, try to reconnect it
        if (wifiMulti.run() != WL_CONNECTED) {
            Serial.println("Wifi connection lost");
        }

        // Write point 1
        if (!client.writePoint(bms_voltage)) {
            Serial.print("InfluxDB write failed: ");
            Serial.println(client.getLastErrorMessage());
        }
        // Write point 2
        if (!client.writePoint(bms_temps)) {
            Serial.print("InfluxDB write failed: ");
            Serial.println(client.getLastErrorMessage());
        }
        // Write point 3
        if (!client.writePoint(bms_stats)) {
            Serial.print("InfluxDB write failed: ");
            Serial.println(client.getLastErrorMessage());
        }
        // Write point 4
        if (!client.writePoint(bms_balStatus)) {
            Serial.print("InfluxDB write failed: ");
            Serial.println(client.getLastErrorMessage());
        }

        // Reset new Msg Flag 
        newMsgFlag = false;
    }

}