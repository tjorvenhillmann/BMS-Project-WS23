#include <Arduino.h>
#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

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

// Data point
Point sensor("wifi_status");

// Real data points
Point bms_data("bms_status");
Point bms_temps("bms_temps");

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

struct Rest
{
  float current = 0;
  uint16_t soc = 0;
  uint16_t soh = 0; // %
  uint16_t cap = 0; // mAh
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
Rest rest;
Status status;

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

  // Add tags
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

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
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void loop() {
  // Store measured value into point
  bms_data.clearFields();
  // Report RSSI of currently connected network
  set_random_voltages();
  set_random_temps();
  voltages.pack_voltage = voltages.cell_V1 + voltages.cell_V2 + voltages.cell_V3 + voltages.cell_V4;
  bms_data.addField("cell_V1", voltages.cell_V1);
  bms_data.addField("cell_V2", voltages.cell_V2);
  bms_data.addField("cell_V3", voltages.cell_V3);
  bms_data.addField("cell_V4", voltages.cell_V4);
  bms_data.addField("pack_voltage", voltages.pack_voltage);
  bms_temps.addField("Temp1", temps.temp1);
  bms_temps.addField("Temp2", temps.temp2);
  bms_temps.addField("Temp3", temps.temp3);
  bms_temps.addField("Temp4", temps.temp4);

  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(bms_data));
  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }

  // Write point 1
  if (!client.writePoint(bms_data)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  // Write point 2
  if (!client.writePoint(bms_temps)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  //Wait 10s
  Serial.println("Wait 10s");
  delay(10000);
}