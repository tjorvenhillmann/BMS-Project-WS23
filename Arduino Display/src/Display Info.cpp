//Set the pins to the correct ones for your development shield or breakout board.
//when using the BREAKOUT BOARD only and using these 8 data lines to the LCD,
//pin usage as follow:
//             CS  CD  WR  RD  RST  D0  D1  D2  D3  D4  D5  D6  D7
//Arduino Uno  A3  A2  A1  A0  A4   8   9   2   3   4   5   6   7
//Arduino Mega A3  A2  A1  A0  A4   8   9   2   3   4   5   6   7

//Remember to set the pins to suit your display module!

#include <LCDWIKI_GUI.h> //Core graphics library
#include <LCDWIKI_KBV.h> //Hardware-specific library

LCDWIKI_KBV mylcd(ILI9486,A3,A2,A1,A0,A4); //model,cs,cd,wr,rd,reset

//define some colour values
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

void setup() 
{
  mylcd.Init_LCD();
  mylcd.Fill_Screen(BLACK);
  mylcd.Invert_Display(1); //Needed in Order for the colours to be correct
}

void loop() 
{
  
  mylcd.Set_Text_Mode(0); //Aktiv sein für irgendwelche Stringkacke?
  //mylcd.Fill_Screen(0x0000);
  mylcd.Set_Rotation(1); //Horizontal Usage 
/*
  //display 2 times string
  mylcd.Set_Text_colour(GREEN);
  mylcd.Set_Text_Back_colour(BLACK);
  mylcd.Set_Text_Size(2);
  mylcd.Print_String("Zeile 2", 0, 40);
  mylcd.Print_Number_Float(01234.56789, 2, 0, 56, '.', 0, ' ');  
  mylcd.Print_Number_Int(0xDEADBEF, 0, 72, 0, ' ',16);
  //mylcd.Print_String("DEADBEEF", 0, 72);

  //display 3 times string
  mylcd.Set_Text_colour(BLUE);
  mylcd.Set_Text_Size(3);
  mylcd.Print_String("Zeile 3", 0, 104);
  mylcd.Print_Number_Float(01234.56789, 2, 0, 128, '.', 0, ' ');  
  mylcd.Print_Number_Int(0xDEADBEF, 0, 152, 0, ' ',16);
 // mylcd.Print_String("DEADBEEF", 0, 152);

  //display 4 times string
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(4);
  mylcd.Print_String("Zeile 4", 0, 192);

  //display 5 times string
  mylcd.Set_Text_colour(YELLOW);
  mylcd.Set_Text_Size(5);
  mylcd.Print_String("Zeile 5", 0, 224);
  */
  

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
  mylcd.Print_String("Temp :    C", 10, 5); //Grad über Circle?
  mylcd.Set_Draw_color(WHITE);
  mylcd.Draw_Circle(185,5,3);
  mylcd.Print_String("Curr.:    A", 10, 35);
  mylcd.Print_String("Power:    W", 10, 65);

  //QUADRANT 2 (Cloud, State)
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(3);
  mylcd.Print_String("Cloud:       ", 230, 5);
  mylcd.Print_String("State:      ", 230, 35);

  //QUADRANT 3 (SOC)
  mylcd.Draw_Round_Rectangle(20, 180, 90, 300, 6);



  //QUADRANT 4 (Voltage, Cell voltage)
  mylcd.Set_Text_colour(WHITE);
  mylcd.Set_Text_Size(3);
  
  mylcd.Print_String("Voltage:     V", 230, 150);
  mylcd.Print_String("Cell1:     V", 230, 200);
  mylcd.Print_String("Cell2:     V", 230, 230);
  mylcd.Print_String("Cell3:     V", 230, 260);
  mylcd.Print_String("Cell4:     V", 230, 290);
  


  //VALUES
  mylcd.Print_Number_Int(50, 125, 5, 3, ' ', 10);
  //mylcd.Print_Number_Int(0xDEADBEF, 0, 72, 0, ' ',16);
  //mylcd.Print_Number_Float(01234.56789, 2, 0, 56, '.', 0, ' ');




  //Erase old Values
  //mylcd.Set_Draw_color(RED);
  //mylcd.Draw_Rectangle(50,0,90,130); //Temp, Curr., Power
  //mylcd.Fill_Rectangle(105,0,180,130);



  delay(1000);
}
