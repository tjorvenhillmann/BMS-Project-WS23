//Set the pins to the correct ones for your development shield or breakout board.
//when using the BREAKOUT BOARD only and using these 8 data lines to the LCD,
//pin usage as follow:
//             CS  CD  WR  RD  RST  D0  D1  D2  D3  D4  D5  D6  D7
//Arduino Uno  A3  A2  A1  A0  A4   8   9   2   3   4   5   6   7
//Arduino Mega A3  A2  A1  A0  A4   8   9   2   3   4   5   6   7

//Remember to set the pins to suit your display module!

#include <LCDWIKI_GUI.h> //Core graphics library
#include <LCDWIKI_KBV.h> //Hardware-specific library
#include <math.h> //MATH
#include <SPI.h> //CAN Comm
#include <mcp2515.h> //CAN Comm

LCDWIKI_KBV mylcd(ILI9486,A3,A2,A1,A0,A4); //model,cs,cd,wr,rd,reset

struct can_frame canMsg;
MCP2515 mcp2515(49); // CS PIN from MCP CAN Module

//define some colour values
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

//define Frame ids
int resolution = 100;
byte frame_id_1 = 0x001;  // temperature ID
byte frame_id_2 = 0x002;  // Voltage ID
byte frame_id_3 = 0x003;  // Current SOH
byte frame_id_4 = 0x004;  // Balance Status
byte hexmsg[2] = {0,0};

//Functions
void clearSOC (); //clears all segments
void setSOC(); //sets the 1-5 green segments
bool SOCchange(); //returns true if the SOC changed

void getCANmessage();
float byte2float();

//Global variable
int SOCold = 0; // old SOC
float vol_all = 0; //total voltage
int power = 0; //power

float temp_list[4];
float vol_list[4];
float current_soc_soh_list[4];
int balance_list[4];



void setup() 
{
  Serial.begin(9600); //Debugging
  //CAN
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  //LCD
  mylcd.Init_LCD();
  mylcd.Fill_Screen(BLACK);
  mylcd.Invert_Display(1); //Needed in Order for the colours to be correct
  mylcd.Set_Text_Mode(0); //Aktiv sein für irgendwelche Stringkacke?
  mylcd.Set_Rotation(1); //Horizontal Usage 

    /*
  MASK
  -----------|-----------
  |     1    |     2    |
  |          |          |
  -----------|-----------
  |     3    |     4    |
  |          |          |
  -----------|-----------
  */
  mylcd.Set_Text_Back_colour(BLACK);
  mylcd.Set_Draw_color(WHITE);
  mylcd.Draw_Line(0,140,480,140); //horizontal
  mylcd.Draw_Line(215,0,215,320); //perpendicular

  //QUADRANT 1 (Current, Temp, Power)
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(3);
  mylcd.Print_String("Temp :    C", 10, 5); 
  mylcd.Set_Draw_color(WHITE);
  mylcd.Draw_Circle(185,5,3); //Circle for Celsius
  mylcd.Print_String("Curr.:    A", 10, 35);
  mylcd.Print_String("Power:    W", 10, 65);

  //QUADRANT 2 (Cloud, State)
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(3);
  mylcd.Print_String("Cloud:       ", 230, 5);
  mylcd.Print_String("State:      ", 230, 35);

  //QUADRANT 3 (SOC)
  mylcd.Draw_Round_Rectangle(20, 178, 90, 300, 6);


  //QUADRANT 4 (Voltage, Cell voltage)
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(3);
  
  mylcd.Print_String("Voltage:     V", 230, 150);
  mylcd.Print_String("Cell1:     V", 230, 200);
  mylcd.Print_String("Cell2:     V", 230, 230);
  mylcd.Print_String("Cell3:     V", 230, 260);
  mylcd.Print_String("Cell4:     V", 230, 290);

}

float byte2float(byte * var, int resolution){
  //static int resolution = 100; //ggfs. init danach
  return ((float) *reinterpret_cast<int*>(var))/resolution;
}

void getCANmessage(){
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id == frame_id_1)
    {
      for (int i = 0; i<canMsg.can_dlc; i+=2)  {  // print the data
      //Serial.print("Temp: ");
      hexmsg[1] = canMsg.data[i];
      hexmsg[0] = canMsg.data[i+1];
      temp_list[i/2] = byte2float(hexmsg, 100);
      //Serial.println(temp_list[i/2]);
      }  
    }
    else if (canMsg.can_id == frame_id_2)
    {
      for (int i = 0; i<canMsg.can_dlc; i+=2)  {  // print the data
      //Serial.print("Voltage: ");
      hexmsg[1] = canMsg.data[i];
      hexmsg[0] = canMsg.data[i+1];
      vol_list[i/2] = byte2float(hexmsg, 100);
      //Serial.println(vol_list[i/2]);
      }  
    }
    else if (canMsg.can_id == frame_id_3)
    {
      for (int i = 0; i<canMsg.can_dlc; i+=2)  {  // print the data
      //Serial.println("Current & SOH & SOC & CAPACITY: ");
      hexmsg[1] = canMsg.data[i];
      hexmsg[0] = canMsg.data[i+1];
      current_soc_soh_list[i/2] = byte2float(hexmsg, 100);
      //Serial.println( current_soc_soh_list[i]);
      }      
    }
    else if (canMsg.can_id == frame_id_4)
    {
      for (int i = 0; i<canMsg.can_dlc; i+=1)  {  // print the data
      //Serial.print("Balance Status: ");
      //Serial.println(balance_list[i], HEX);
      }      
    }
  }
}

void clearSOC() {
  mylcd.Set_Draw_color(BLACK);
  mylcd.Fill_Round_Rectangle(21, 179, 89, 299, 6);
  }

void setSOC(int SOC) {

if (SOC >= 0 && SOC <= 20) //20%
  {
    clearSOC(); //Erase all segments
    mylcd.Set_Draw_color(GREEN);
    mylcd.Fill_Round_Rectangle(22,276,88,298,6); //5.
  }
  else if (SOC > 20 && SOC <= 40) //40%
  {
    clearSOC(); //Erase all segments
    mylcd.Set_Draw_color(GREEN);
    mylcd.Fill_Round_Rectangle(22,276,88,298,6); //5. 
    mylcd.Fill_Round_Rectangle(22,252,88,274,6); //4. 

  }
  else if (SOC > 40 && SOC <= 60) //60%
  {
    clearSOC(); //Erase all segments
    mylcd.Set_Draw_color(GREEN);
    mylcd.Fill_Round_Rectangle(22,276,88,298,6); //5. 
    mylcd.Fill_Round_Rectangle(22,252,88,274,6); //4. 
    mylcd.Fill_Round_Rectangle(22,228,88,250,6); //3.

  }
  else if (SOC > 60 && SOC <= 80) //80%
  {
    clearSOC(); //Erase all segments
    mylcd.Set_Draw_color(GREEN);
    mylcd.Fill_Round_Rectangle(22,276,88,298,6); //5. 
    mylcd.Fill_Round_Rectangle(22,252,88,274,6); //4. 
    mylcd.Fill_Round_Rectangle(22,228,88,250,6); //3. 
    mylcd.Fill_Round_Rectangle(22,204,88,226,6); //2.

  }
  else if (SOC > 80 && SOC <= 100)
  {
    clearSOC(); //Erase all segments
    mylcd.Set_Draw_color(GREEN);
    mylcd.Fill_Round_Rectangle(22,276,88,298,6); //5. 
    mylcd.Fill_Round_Rectangle(22,252,88,274,6); //4. 
    mylcd.Fill_Round_Rectangle(22,228,88,250,6); //3. 
    mylcd.Fill_Round_Rectangle(22,204,88,226,6); //2.
    mylcd.Fill_Round_Rectangle(22,180,88,202,6); //1.

  }
}

bool SOCchange(int SOCnew) {
SOCnew = (SOCnew-1) /20; // (-1 - 99) -> -1 - 4
switch (SOCnew) {
  case 0: SOCnew = 1; break;
  case 1: SOCnew = 2; break;
  case 2: SOCnew = 3; break;
  case 3: SOCnew = 4; break;
  case 4: SOCnew = 5; break;
  default: clearSOC(); break;
}

if (SOCnew != SOCold) {
  SOCold = SOCnew;
  return true;
}
else {
  return false;
}

}


void loop() 
{
  //VALUES 
  getCANmessage();

  //Quadrant 1 (Current, Temp, Power)
  power = round((vol_list[0] + vol_list[1] + vol_list[2] + vol_list[3]) * current_soc_soh_list[0]); 
  mylcd.Set_Text_colour(WHITE);
  mylcd.Print_Number_Int(round(temp_list[1]), 125, 5, 2, ' ', 10); //Temp
  mylcd.Print_Number_Float(current_soc_soh_list[0], 1, 125, 35, '.', 2, ' '); //Current
  mylcd.Print_Number_Int(power, 125, 65, 2, ' ', 10); //Power 

  //Quadrant 2 (Cloud, State)

  if (true)
  {
    mylcd.Set_Text_colour(GREEN);
    mylcd.Print_String("online", 350, 5);
  }
  else
  {
    mylcd.Set_Text_colour(RED);
    mylcd.Print_String("offline", 350, 5);
  }

  switch (1)
  {
  case 1:
    mylcd.Set_Text_colour(RED);
    mylcd.Print_String("Error", 350, 35);
    break;
  case 2:
    mylcd.Set_Text_colour(YELLOW);
    mylcd.Print_String("Warning", 350, 35);
    break;
  case 3:
    mylcd.Set_Text_colour(GREEN);
    mylcd.Print_String("None", 350, 35);
    break;
  }

  //Quadrant 3 (SOC)

  mylcd.Set_Text_colour(WHITE);
  mylcd.Print_String("SOC", 30, 150);
  if (SOCchange(current_soc_soh_list[1]) == true) {
    setSOC(current_soc_soh_list[1]);
  }
  

  //Quadrant 4 (Voltage, Cell voltage)

  mylcd.Set_Text_colour(WHITE);
  vol_all = vol_list[0] + vol_list[1] + vol_list[2] + vol_list[3]; //Total voltage 11.20 V

  mylcd.Print_Number_Float(vol_all, 1, 380, 150, '.', 2, ' '); //Total Voltage
  mylcd.Print_Number_Float(vol_list[0], 2, 340, 200, '.', 2, ' '); //Cell 1
  mylcd.Print_Number_Float(vol_list[1], 2, 340, 230, '.', 2, ' '); //Cell 2
  mylcd.Print_Number_Float(vol_list[2], 2, 340, 260, '.', 2, ' '); //Cell 3
  mylcd.Print_Number_Float(vol_list[3], 2, 340, 290, '.', 2, ' '); //Cell 4
  

  
  delay(1000); //Aktualisierungsdelay
}
