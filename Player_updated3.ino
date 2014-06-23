#include <glcd.h>
#include <fonts/allFonts.h>
#include <VirtualWire.h>

const int PERIOD = 2;
const int THRESHOLD = 700;
const int MAX_GUNS = 7;
const int MAX_MODES = 3;
const int MAX_PLAYERS = 16;
const int MAX_HELMETS = 4;
const int MAX_VESTS = 4;
const int INFINITE_INT = 32766;
const float HEADSHOT = 1.5;
const int BUTTON_DELAY = 200;
const int HIT_POINTS = 500;
int BROADCAST = MAX_PLAYERS;

//Input Pins
const int vest_inputPin = A5;       //pull down
const int helmet_inputPin = 0;      //pull down

//Output Pins
const int laserPin = 7;

//Buttons
const int noButton = 0;
const int triggerPin = 13;      //pull down
const int reloadPin = 52;        //pull down                  also Select key
const int shopPin = 49;          //                           also Home key
const int equipsPin = 50;        //                           also Back key
const int scrollLeft = 43;
const int scrollRight = 47;
const int rf_transmitPin = 53;
const int rf_receivePin = 2;

//Screens
const int Home = 0;
const int Shop = 1;
const int Equips = 2;
const int gunUpdate = 3;
const int modeUpdate = 4;
const int shop_ammo = 10;
const int shop_weapon = 11;
const int shop_armour = 12;
const int shop_helmet = 13;
const int shop_medics = 14;
const int equips_profile = 20;
const int equips_weapon = 21;
const int equips_armour = 22;
const int equips_medics = 23;
int Screen = Home;

//Message types
const int HIT = 1;
const int Init = 2;

//Other Constants
boolean updateLcd = 1;
boolean NOTIFY = 0;
int notifyTime = 2000;    //mili seconds
unsigned long time;

typedef struct
{
  int code;
  char name[15]; 
  char short_name[3]; 
  int recoil_time;
  int Xammo;
  int Xhealth;
}Mode;

typedef struct
{
  int code;
  char name[11];
  int cost;
  int no_of_modes;
  Mode mode[MAX_MODES];
  Mode curr_mode;
  int full_ammo;
  int reload_time;
  int curr_ammo;
  int total_ammo;
}Gun;

typedef struct
{
  int code;
  char name[15];
  int cost;
  float defence_factor;
  int health;
  int decay_rate;
}Helmet;

typedef struct
{
  int code;
  char name[20];
  int cost;
  float defence_factor;
  int health;
  int decay_rate;
}Vest;

typedef struct
{
  int no_available;
  int health_factor;
  int cost;
}Medics;

typedef struct
{
  int no_of_guns;
  Medics medics;
  Gun My_Guns[MAX_GUNS];
  Helmet My_Helmet;
  Vest My_Vest;  
}Inventory;

struct
{
  byte code;
  char name[12];
  char team[12];
  int Money;
  int Health;
  Gun curr_weapon;
  Inventory Equips;
}Player;

Gun gun[MAX_GUNS];
Helmet helmet[MAX_HELMETS];
Vest vest[MAX_VESTS];
Medics medics;

void setup()
{
  //Serial.begin(9600);
  //Serial.println("Health: 100");
  
  GLCD.Init();
  GLCD.ClearScreen();
  GLCD.SelectFont(SystemFont5x7);
  
  vw_set_tx_pin(rf_transmitPin);
  vw_set_rx_pin(rf_receivePin);
  vw_setup(2000); 
  vw_rx_start();
  
  pinMode(vest_inputPin, INPUT);
  pinMode(helmet_inputPin, INPUT);
  pinMode(laserPin, OUTPUT);
  pinMode(triggerPin, INPUT);
  pinMode(reloadPin, INPUT);
  pinMode(shopPin, INPUT);
  pinMode(equipsPin, INPUT);
  pinMode(scrollLeft, INPUT);
  pinMode(scrollRight, INPUT);
  
  database();
  initialize();
  //initialize lcd
}

void loop()          //line number: 124
{
 
  if((Screen == Home)||(Screen == gunUpdate)||(Screen == modeUpdate))
    sense_shot(updateLcd);
  else
    sense_shot(!updateLcd);
     
  rf_receive();
  
  if((digitalRead(triggerPin) == HIGH) && (Screen == Home)) 
  {
    unsigned long int button_press_time = millis();
    send_data();
    if(Player.curr_weapon.curr_mode.recoil_time > 0)
      while((millis() - button_press_time) <= Player.curr_weapon.curr_mode.recoil_time)
        sense_shot(!updateLcd);
    else
      while(((millis() - button_press_time) <= -Player.curr_weapon.curr_mode.recoil_time) || (digitalRead(triggerPin) == HIGH))
        sense_shot(!updateLcd);  
  }
  
  if(digitalRead(reloadPin) == HIGH)
  {
    if(Screen == Home)
      reload();
    else
      choose_func(Screen, reloadPin);
    delay(BUTTON_DELAY);
  }
   
  if(digitalRead(shopPin) == HIGH)
  {
    if(Screen == Home)
    {
      Screen = Shop;
      lcd_update(Screen);
    }
    else
    {
      Screen = Home;
      lcd_update(Screen);
    }
    delay(BUTTON_DELAY);
  }
  
  if(digitalRead(equipsPin) == HIGH)
  {
    if(Screen == Home)
    {
      Screen = Equips;
      lcd_update(Screen);
    }
    else
    {
      Screen = Screen/10;
      lcd_update(Screen);
    }
    delay(BUTTON_DELAY);
  }
  
  if(digitalRead(scrollLeft) == HIGH)          
  {
    if(Screen == Home)
    {
      Screen = modeUpdate;
      home_mode_change(noButton);
    }
    else
      choose_func(Screen, scrollLeft);
    delay(BUTTON_DELAY);
  }
  
  if(digitalRead(scrollRight) == HIGH)          
  {
    if(Screen == Home)
    {
      Screen = gunUpdate;
      home_gun_change(noButton);
    }
    else
      choose_func(Screen, scrollRight);
    delay(BUTTON_DELAY);
  }
  
  if(NOTIFY)
  {
    if((millis() - time) > notifyTime)
      NOTIFY = !NOTIFY;
  }
}  

void database()
{  
  //Gun 0
  
  gun[0].code = 0;
  strcpy(gun[0].name, "DefaultGun");
  gun[0].cost = 0;
  gun[0].no_of_modes = 1;
  
  gun[0].mode[0].code = 0;
  strcpy(gun[0].mode[0].name, "Single Shot");
  strcpy(gun[0].mode[0].short_name, "SS");
  gun[0].mode[0].recoil_time = -950;
  gun[0].mode[0].Xammo = 1;
  gun[0].mode[0].Xhealth = 8;
   
  gun[0].curr_mode = gun[0].mode[0];
  gun[0].full_ammo = 10;
  gun[0].reload_time = 3500;
  gun[0].curr_ammo = 10;
  gun[0].total_ammo = INFINITE_INT;
  
  
  //Gun 1
  
  gun[1].code = 1;
  strcpy(gun[1].name, "Pistol");
  gun[1].cost = 0;
  gun[1].no_of_modes = 2;
  
  gun[1].mode[0].code = 0;
  strcpy(gun[1].mode[0].name, "Single Shot");
  strcpy(gun[1].mode[0].short_name, "SS");
  gun[1].mode[0].recoil_time = -850;
  gun[1].mode[0].Xammo = 1;
  gun[1].mode[0].Xhealth = 8;
  
  gun[1].mode[1].code = 1;
  strcpy(gun[1].mode[1].name, "Automatic");
  strcpy(gun[1].mode[1].short_name, "A");
  gun[1].mode[1].recoil_time = 500;
  gun[1].mode[1].Xammo = 1;
  gun[1].mode[1].Xhealth = 10;
   
  gun[1].curr_mode = gun[1].mode[0];
  gun[1].full_ammo = 15;
  gun[1].reload_time = 2500;
  gun[1].curr_ammo = 15;
  gun[1].total_ammo = 30;  
  
  
  //Gun 2
  
  gun[2].code = 2;
  strcpy(gun[2].name, "Shotgun");
  gun[2].cost = 2000;
  gun[2].no_of_modes = 1;
  
  gun[2].mode[0].code = 0;
  strcpy(gun[2].mode[0].name, "Single Shot");
  strcpy(gun[2].mode[0].short_name, "SS");
  gun[2].mode[0].recoil_time = 0;
  gun[2].mode[0].Xammo = 1;
  gun[2].mode[0].Xhealth = 25;
   
  gun[2].curr_mode = gun[2].mode[0];
  gun[2].full_ammo = 1;
  gun[2].reload_time = 1000;
  gun[2].curr_ammo = 1;
  gun[2].total_ammo = 10;
  
  
  //Gun 3
  
  gun[3].code = 3;
  strcpy(gun[3].name, "SMG");
  gun[3].cost = 1600;
  gun[3].no_of_modes = 3;
  
  gun[3].mode[0].code = 0;
  strcpy(gun[3].mode[0].name, "Single Shot");
  strcpy(gun[3].mode[0].short_name, "SS");
  gun[3].mode[0].recoil_time = -700;
  gun[3].mode[0].Xammo = 1;
  gun[3].mode[0].Xhealth = 15;
  
  gun[3].mode[1].code = 1;
  strcpy(gun[3].mode[1].name, "Burst Fire");
  strcpy(gun[3].mode[1].short_name, "BF");
  gun[3].mode[1].recoil_time = -1000;
  gun[3].mode[1].Xammo = 3;
  gun[3].mode[1].Xhealth = 30;
   
  gun[3].mode[2].code = 2;
  strcpy(gun[3].mode[2].name, "Automatic");
  strcpy(gun[3].mode[2].short_name, "A");
  gun[3].mode[2].recoil_time = 150;
  gun[3].mode[2].Xammo = 1;
  gun[3].mode[2].Xhealth = 12;
  
  gun[3].curr_mode = gun[3].mode[2];
  gun[3].full_ammo = 25;
  gun[3].reload_time = 1500;
  gun[3].curr_ammo = 25;
  gun[3].total_ammo = 40;
  
  
  //Gun 4
  
  gun[4].code = 4;
  strcpy(gun[4].name, "Rifle");
  gun[4].cost = 2400;
  gun[4].no_of_modes = 2;
  
  gun[4].mode[0].code = 0;
  strcpy(gun[4].mode[0].name, "Single Shot");
  strcpy(gun[4].mode[0].short_name, "SS");
  gun[4].mode[0].recoil_time = -1000;
  gun[4].mode[0].Xammo = 1;
  gun[4].mode[0].Xhealth = 20;
  
  gun[4].mode[1].code = 1;
  strcpy(gun[4].mode[1].name, "Burst Fire");
  strcpy(gun[4].mode[1].short_name, "BF");
  gun[4].mode[1].recoil_time = -1300;
  gun[4].mode[1].Xammo = 3;
  gun[4].mode[1].Xhealth = 45;
   
  gun[4].curr_mode = gun[4].mode[0];
  gun[4].full_ammo = 10;
  gun[4].reload_time = 3000;
  gun[4].curr_ammo = 10;
  gun[4].total_ammo = 15;
  
  
  //Gun 5
  
  gun[5].code = 5;
  strcpy(gun[5].name, "Sniper");
  gun[5].cost = 2250;
  gun[5].no_of_modes = 1;
  
  gun[5].mode[0].code = 0;
  strcpy(gun[5].mode[0].name, "Single Shot");
  strcpy(gun[5].mode[0].short_name, "SS");
  gun[5].mode[0].recoil_time = 0;
  gun[5].mode[0].Xammo = 1;
  gun[5].mode[0].Xhealth = 50;
   
  gun[5].curr_mode = gun[5].mode[0];
  gun[5].full_ammo = 1;
  gun[5].reload_time = 2000;
  gun[5].curr_ammo = 1;
  gun[5].total_ammo = 7;
  
  
  //Gun 6
  
  gun[6].code = 6;
  strcpy(gun[6].name, "MachineGun");
  gun[6].cost = 2500;
  gun[6].no_of_modes = 2;
  
  gun[6].mode[0].code = 0;
  strcpy(gun[6].mode[0].name, "Automatic");
  strcpy(gun[6].mode[0].short_name, "A");
  gun[6].mode[0].recoil_time = 75;
  gun[6].mode[0].Xammo = 1;
  gun[6].mode[0].Xhealth = 15;
  
  gun[6].mode[1].code = 1;
  strcpy(gun[6].mode[1].name, "Burst Fire");
  strcpy(gun[6].mode[1].short_name, "BF");
  gun[6].mode[1].recoil_time = -200;
  gun[6].mode[1].Xammo = 3;
  gun[6].mode[1].Xhealth = 30;
   
  gun[6].curr_mode = gun[6].mode[0];
  gun[6].full_ammo = 60;
  gun[6].reload_time = 5000;
  gun[6].curr_ammo = 60;
  gun[6].total_ammo = 100;
  
  
  //Armour 0
  
  vest[0].code = 0;
  strcpy(vest[0].name, "No Armour");
  vest[0].cost = 0;
  vest[0].defence_factor = 1;
  vest[0].health = INFINITE_INT;
  vest[0].decay_rate = 0;
  
  
  //Armour 1
  
  vest[1].code = 1;
  strcpy(vest[1].name, "Light Steel Armour");
  vest[1].cost = 500;
  vest[1].defence_factor = 0.30;
  vest[1].health = 100;
  vest[1].decay_rate = 25;
  
  
  //Armour 2
  
  vest[2].code = 2;
  strcpy(vest[2].name, "Rough Steel Armour");
  vest[2].cost = 1000;
  vest[2].defence_factor = 0.55;
  vest[2].health = 100;
  vest[2].decay_rate = 20;
  
  
  //Armour 3
  
  vest[3].code = 3;
  strcpy(vest[3].name, "Heavy Steel Armour");
  vest[3].cost = 1500;
  vest[3].defence_factor = 0.80;
  vest[3].health = 100;
  vest[3].decay_rate = 17;
  
  
  //Helmet 0
  
  helmet[0].code = 0;
  strcpy(helmet[0].name, "No Helmet");
  helmet[0].cost = 0;
  helmet[0].defence_factor = 1;
  helmet[0].health = INFINITE_INT;
  helmet[0].decay_rate = 0;
  
  
  //Helmet 1
  
  helmet[1].code = 1;
  strcpy(helmet[1].name, "Light Steel Helmet");
  helmet[1].cost = 250;
  helmet[1].defence_factor = 0.30;
  helmet[1].health = 100;
  helmet[1].decay_rate = 50;
  
  
  //Helmet 2
  
  helmet[2].code = 2;
  strcpy(helmet[2].name, "Rough Steel Helmet");
  helmet[2].cost = 700;
  helmet[2].defence_factor = 0.55;
  helmet[2].health = 100;
  helmet[2].decay_rate = 34;
  
  
  //Helmet 3
  
  helmet[3].code = 3;
  strcpy(helmet[3].name, "Heavy Steel Helmet");
  helmet[3].cost = 1200;
  helmet[3].defence_factor = 0.80;
  helmet[3].health = 100;
  helmet[3].decay_rate = 25;
  
  //Medics
  
  medics.no_available = 0;
  medics.cost = 500;
  medics.health_factor = 10;
  
}

void initialize()
{
  //Get inital stats from the centre
  Player.code = 0;
  //strcpy(Player.name, "Satyam Dwi");
  //strcpy(Player.team, "Summer Shk");
  Player.Money = 5000; //Default money
  Player.Health = 100; //Default Health
  Player.Equips.no_of_guns = 2;
  Player.Equips.medics = medics;
  Player.Equips.My_Guns[0] = gun[0];
  Player.Equips.My_Guns[1] = gun[1];
  Player.Equips.My_Vest = vest[0];
  Player.Equips.My_Helmet = helmet[0];
  
  Player.curr_weapon = Player.Equips.My_Guns[0];
  
  lcd_update(Screen);
}

int receive_data(int inputPin, byte* player_code)
{
  int Xhealth = 0;
  int count = MAX_PLAYERS/2;
    
  delay(PERIOD*3/2);
  while(count > 0)
  {
    if(analogRead(inputPin) > THRESHOLD)
    {
      *player_code = 2*(*player_code) + 1;
    }
    else
    {
      *player_code = 2*(*player_code);
    }
    count = count/2;
    delay(PERIOD);
  }
   
  count = 7;
  
  while(count > 0)
  {
    if(analogRead(inputPin) > THRESHOLD)
    {
      Xhealth = 2*Xhealth + 1;
    }
    else
    {
      Xhealth = 2*Xhealth;
    }
    count--;
    delay(PERIOD);
  }
  /*Serial.println(player_code);  
  Serial.println(Xhealth);*/
  delay(PERIOD);
  
  //send to that player you hit!
  return Xhealth;
}

void send_data()
{
  byte val = Player.code;
  int fac = MAX_PLAYERS/2;
  
  if(Player.curr_weapon.curr_ammo < Player.curr_weapon.curr_mode.Xammo)
  {  //return;    //lcd print insufficient ammo, buzzer sound 
     Notify("No ammo");
     //delay(3000);
     lcd_update(Screen);
     return;
  }
  //buzzer sound  
  digitalWrite(laserPin, HIGH);
  delay(PERIOD);
  
  while(fac > 0)
  {
    if(val/fac == 1)
    {
      digitalWrite(laserPin, HIGH);
      delay(PERIOD);
    }
    else
    {
      digitalWrite(laserPin, LOW);
      delay(PERIOD);
    }
    val = val % fac;
    fac = fac / 2;
  }
  
  val = Player.curr_weapon.curr_mode.Xhealth;
  fac = 64;
  
  while(fac > 0)
  {
    if(val/fac == 1)
    {
      digitalWrite(laserPin, HIGH);
      delay(PERIOD);
    }
    else
    {
      digitalWrite(laserPin, LOW);
      delay(PERIOD);
    }
    val = val % fac;
    fac = fac / 2;
  }
  
  digitalWrite(laserPin, LOW);
  delay(PERIOD);
  
  Player.curr_weapon.curr_ammo -= Player.curr_weapon.curr_mode.Xammo;
  Player.curr_weapon.total_ammo -= Player.curr_weapon.curr_mode.Xammo;
  
  lcd_update(Screen);   
}

void rf_receive()
{
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;

  if(vw_get_message(buf, &buflen)) // Non-blocking
  {
    if((buf[0] != BROADCAST) || (buf[0] != Player.code))
      return;
      
    byte j, i = 0;
    byte playerCode, msgType;   
    
    playerCode = buf[1];
    msgType = buf[2];    
    
    switch(msgType)
    {
      case HIT:  
        Notify("YOU HIT");
        Player.Money += HIT_POINTS;
        break;
      case Init:
        for(i = 0; i < buf[3]; ++i)
          Player.name[i] = buf[i+4];
        for(j = i; j < buf[i]; ++j)
          Player.team[j-i] = buf[j+5];        
    }
    //for(i = 0; i < buflen - 3; ++i){}
  }  
}

void rf_transmit(byte receiver_address, byte msg_type, byte msg[], int len)
{
  byte i = 0;
  byte msg_encrypt[VW_MAX_MESSAGE_LEN];
  msg_encrypt[0] = receiver_address;
  msg_encrypt[1] = Player.code;
  msg_encrypt[2] = msg_type;
  for(i = 0; i < len; ++i)
    msg_encrypt[i+3] = msg[i];
  vw_send((uint8_t *)msg_encrypt, len+3);
  vw_wait_tx();
}

void reload()
{
  unsigned long time = millis();
 
  Player.curr_weapon.total_ammo -= (Player.curr_weapon.full_ammo - Player.curr_weapon.curr_ammo);
  Player.curr_weapon.curr_ammo = Player.curr_weapon.full_ammo;
  GLCD.ClearScreen();
  GLCD.println("LOADING....."); 
  GLCD.DrawRect(9,24,80,12);
  while((millis() - time) <= Player.curr_weapon.reload_time)
  {
    GLCD.FillRect(11,26,map((millis() - time), 0, Player.curr_weapon.reload_time, 0, 76), 8);
    sense_shot(!updateLcd);
  }
  
  lcd_update(Screen);
}
  
void sense_shot(int type)
{
    int val;
    byte playerCode;
      
    if(analogRead(vest_inputPin) > THRESHOLD)
    {
       val = receive_data(vest_inputPin, &playerCode);
       Player.Health -= Player.Equips.My_Vest.defence_factor * val;
       if(Player.Health <= 0)
       {
         GLCD.ClearScreen();
         GLCD.print("YOU ARE DEAD!");
         delay(9999999);
       }
       Player.Equips.My_Vest.health -= Player.Equips.My_Vest.decay_rate;
       if(Player.Equips.My_Vest.health <= 0)
         Player.Equips.My_Vest = vest[0];
       if(type == updateLcd)
       {
         lcd_update(Screen);       
         if(Screen == gunUpdate)
            home_gun_change(noButton);
         if(Screen == modeUpdate)
            home_mode_change(noButton);
       }
       
       rf_transmit(playerCode, HIT, NULL, 0);        
       
    }
    
  /*  if(analogRead(helmet_inputPin) > THRESHOLD)
    {
       val = receive_data(helmet_inputPin, &playerCode);
       Player.Health -= Player.Equips.My_Helmet.defence_factor * HEADSHOT * val;
       if(Player.Health <= 0)
       {
         GLCD.ClearScreen();
         GLCD.print("YOU ARE DEAD!");
         delay(9999999);
       }
       Player.Equips.My_Helmet.health -= Player.Equips.My_Helmet.decay_rate;
       if(Player.Equips.My_Helmet.health <= 0)
         Player.Equips.My_Helmet = helmet[0];
       if(type == updateLcd)
       {
         lcd_update(Screen);       
         if(Screen == gunUpdate)
            home_gun_update(noButton);
         if(Screen == modeUpdate)
            home_mode_update(noButton)
       }
       
       rf_transmit(playerCode, HIT, NULL, 0);
    }  
    */
}
  
void lcd_update(int type)
{
  GLCD.ClearScreen();
  
  if((type == Home)||(type == gunUpdate)||(type == modeUpdate)) 
  {
    //Player And Team Name
    GLCD.println(Player.name);
    GLCD.println(Player.team);
  
    //Gun 
    GLCD.CursorTo(0,3);
    GLCD.print(Player.curr_weapon.name);
    GLCD.print(" (");
    GLCD.print(Player.curr_weapon.curr_mode.short_name);
    GLCD.println(")");
  
   
    GLCD.CursorTo(0,4);
    GLCD.print(Player.curr_weapon.curr_ammo);
    GLCD.print("/");
    GLCD.print(Player.curr_weapon.full_ammo);
    GLCD.GotoXY((29 + 6*(Player.curr_weapon.curr_ammo > 9)),32);
    GLCD.print("of");
    GLCD.GotoXY((44 + 6*(Player.curr_weapon.curr_ammo > 9)),32);
    if(Player.curr_weapon.total_ammo > INFINITE_INT/2)
      GLCD.println("Infinite");
    else
      GLCD.println(Player.curr_weapon.total_ammo);
  
    //Armour 
    print_armour(0, 45);
  
    //Armour Health
    if(Player.Equips.My_Vest.health <= 100)
    {
      GLCD.DrawRect(9,44,50,8);
      GLCD.FillRect(11,46,map(Player.Equips.My_Vest.health, 0, 100, 0, 46),4);
    }
    else
    {
      GLCD.GotoXY(9,44);
      GLCD.print("No Armour");
    }  
    
    if(Player.Equips.My_Helmet.health <= 100)
    {
      GLCD.DrawRect(9,55,50,8);
      GLCD.FillRect(11,57,map(Player.Equips.My_Helmet.health, 0, 100, 0, 46),4);
    }
    else
    {
      GLCD.GotoXY(9,55);
      GLCD.print("No Helmet");
    }    
    
    // Heart
  
    print_heart(77, 0);
    GLCD.GotoXY(85,0);
    GLCD.print(Player.Health);
    GLCD.print("/100");
  
    // Money
  
    print_rupee(77,10);
    GLCD.GotoXY(85, 10);
    GLCD.print(Player.Money);
  
    //Helmet
    print_helmet(0,56);
    
    //notification
    if(NOTIFY)
    {
      Notify(NULL);
    }
  }  
  else
    choose_func(type, noButton);
}

void print_heart(int x, int y)    //width 5
{
  GLCD.DrawRect(x,y+1,4,2);
  GLCD.DrawHLine(x+1,y+2,2);
  GLCD.SetDot(x+1,y,BLACK);
  GLCD.SetDot(x+3,y,BLACK);
  GLCD.SetDot(x+1,y+4,BLACK);
  GLCD.SetDot(x+2,y+4,BLACK);
  GLCD.SetDot(x+3,y+4,BLACK);
  GLCD.SetDot(x+2,y+5,BLACK);
}

void print_armour(int x, int y)    //width 5
{
  GLCD.DrawRect(x,y+1,4,2);
  GLCD.DrawHLine(x+1,y+2,2);
  GLCD.SetDot(x,y,BLACK);
  GLCD.SetDot(x+2,y,BLACK);
  GLCD.SetDot(x+4,y,BLACK);
  GLCD.SetDot(x+1,y+4,BLACK);
  GLCD.SetDot(x+2,y+4,BLACK);
  GLCD.SetDot(x+3,y+4,BLACK);
  GLCD.SetDot(x+2,y+5,BLACK);
}

void print_rupee(int x, int y)    //width 5
{
  GLCD.DrawHLine(x,y,4);
  GLCD.DrawHLine(x,y+2,4);
  GLCD.DrawVLine(x+2,y,3);
  GLCD.DrawHLine(x,y+4,1);
  GLCD.SetDot(x+2,y+5,BLACK);
  GLCD.SetDot(x+3,y+6,BLACK);
  GLCD.SetDot(x+4,y+7,BLACK);
}

void print_helmet(int x, int y)      //width 5
{
  GLCD.DrawHLine(x,y+2,4);
  GLCD.DrawHLine(x+1,y+1,2);
  GLCD.DrawVLine(x+2,y,3);
  GLCD.DrawVLine(x,y+2,3);
  GLCD.DrawVLine(x+4,y+2,3);
}    

void choose_func(int screen, int buttonPressed)
{
  switch(screen)
  {
    case Shop:  show_shop(buttonPressed);  break;
    case Equips:  show_equips(buttonPressed);  break;
    case gunUpdate:  home_gun_change(buttonPressed);  break;
    case modeUpdate:  home_mode_change(buttonPressed);  break;
    case shop_ammo:  Sammo(buttonPressed);  break;
    case shop_weapon:  Sweapon(buttonPressed);  break;
    case shop_armour:  Sarmour(buttonPressed);  break;
    case shop_helmet:  Shelmet(buttonPressed);  break;
    case shop_medics:  Smedics(buttonPressed);  break;
    case equips_profile:  Eprofile(buttonPressed);  break;
    case equips_weapon:  Eweapon(buttonPressed);  break;
    case equips_armour:  Earmour(buttonPressed);  break;
    case equips_medics:  Emedics(buttonPressed);  break;
  }
}

void home_gun_change(int buttonPressed)
{
   static int index = 0;
   
   if(buttonPressed == noButton)
   {
     GLCD.FillRect(0,21,78,12, WHITE);
     GLCD.DrawRect(0,21,78,12);
     GLCD.GotoXY(2,24);
     GLCD.print("<");
     GLCD.GotoXY(10,24);
     GLCD.print(Player.Equips.My_Guns[index].name);
     GLCD.GotoXY(72,24);
     GLCD.print(">");
   }
   
    if(buttonPressed == scrollLeft)
   {
     index--;
     if(index < 0)  
       index = Player.Equips.no_of_guns - 1;
     home_gun_change(noButton);
   }
   
   if(buttonPressed == scrollRight)
   {
     index++;
     if(index == Player.Equips.no_of_guns)  
       index = 0;
     home_gun_change(noButton);
   }
     
   if(buttonPressed == reloadPin)
   {
     Player.curr_weapon = Player.Equips.My_Guns[index];
     Screen = Home;
     lcd_update(Screen);
   }
}

void home_mode_change(int buttonPressed)
{
   static int index = 0;
   
   if(buttonPressed == noButton)
   {
     GLCD.FillRect(strlen(Player.curr_weapon.name)*6+2,21,30,12, WHITE);
     GLCD.DrawRect(strlen(Player.curr_weapon.name)*6+2,21,30,12);
     GLCD.GotoXY(strlen(Player.curr_weapon.name)*6+4,24);
     if(Player.curr_weapon.no_of_modes == 1)
       GLCD.print(" ");
     else
       GLCD.print("<");
     GLCD.GotoXY(strlen(Player.curr_weapon.name)*6+12,24);
     GLCD.print(Player.curr_weapon.mode[index].short_name);
     GLCD.GotoXY(strlen(Player.curr_weapon.name)*6+26,24);
     if(Player.curr_weapon.no_of_modes != 1)
       GLCD.print(">");
   }
   
    if(buttonPressed == scrollLeft)
   {
     index--;
     if(index < 0)  
       index = Player.curr_weapon.no_of_modes - 1;
     home_mode_change(noButton);
   }
   
   if(buttonPressed == scrollRight)
   {
     index++;
     if(index == Player.curr_weapon.no_of_modes)  
       index = 0;
     home_mode_change(noButton);
   }
     
   if(buttonPressed == reloadPin)
   {
     Player.curr_weapon.curr_mode = Player.curr_weapon.mode[index];
     Screen = Home;
     lcd_update(Screen);
   }
}

void show_shop(int buttonPressed)
{
  static int index = 0;
  
  if(buttonPressed == noButton)
  {
    GLCD.ClearScreen();
    
    GLCD.GotoXY(50,0);
    GLCD.print("Shop");
    
    print_heart(82, 10);
    GLCD.GotoXY(90,10);
    GLCD.print(Player.Health);
    
    print_rupee(82,20);
    GLCD.GotoXY(90, 20);
    GLCD.print(Player.Money);
    
    
    if(index == 0)
      GLCD.DrawRect(3,9,45,11);
    
    GLCD.DrawRect(5,13,2,2);
    GLCD.SetDot(6,14,BLACK);
    GLCD.GotoXY( 14,11);
    GLCD.print("Ammo");
    
    if(index == 1)
      GLCD.DrawRect(3,20,50,11);
    
    GLCD.DrawRect(5,24,2,2);
    GLCD.SetDot(6,25,BLACK);
    GLCD.GotoXY( 14,22);
    GLCD.print("Weapon");
    
    if(index == 2)
      GLCD.DrawRect(3,31,42,11);
    
    GLCD.DrawRect(5,35,2,2);
    GLCD.SetDot(6,36,BLACK);
    GLCD.GotoXY( 14,33);
    GLCD.print("Vest");
    
    if(index == 3)
      GLCD.DrawRect(3,42,50,11);
    
    GLCD.DrawRect(5,46,2,2);
    GLCD.SetDot(6,47,BLACK);
    GLCD.GotoXY( 14,44);
    GLCD.print("Helmet");
    
    
    if(index == 4)
      GLCD.DrawRect(3,53,50,10);
    
    GLCD.DrawRect(5,57,2,2);
    GLCD.SetDot(6,58,BLACK);
    GLCD.GotoXY( 14,55);
    GLCD.print("Medics");
  }
  
  if(buttonPressed == scrollLeft)
   {
     index--;
     if(index < 0)  
       index = 4;
     show_shop(noButton);
   }
   
   if(buttonPressed == scrollRight)
   {
     index++;
     if(index > 4)  
       index = 0;
     show_shop(noButton);
   }
   
   if(buttonPressed == reloadPin)
   {
     Screen = Screen*10 + index;
     lcd_update(Screen);
   }
}

void show_equips(int buttonPressed)
{
  static int index = 0;
  
  if(buttonPressed == noButton)
  {
    GLCD.ClearScreen();
    
    GLCD.GotoXY(37,0);
    GLCD.print("Inventory");
    
    print_heart(5,15);
    GLCD.GotoXY(12,15);
    GLCD.print(Player.Health);
    
    print_rupee(90,15);
    GLCD.GotoXY(97,15);
    GLCD.print(Player.Money);
    
    GLCD.DrawRect(10-3*(index==0),30-3*(index==0), 45+6*(index==0),11+6*(index==0));
    GLCD.GotoXY(12,32);
    GLCD.print("Profile");
    
    GLCD.DrawRect(70-3*(index==1),30-3*(index==1), 45+6*(index==1),11+6*(index==1));
    GLCD.GotoXY(75,32);
    GLCD.print("Weapon");
    
    GLCD.DrawRect(10-3*(index==2),47-3*(index==2), 45+6*(index==2),11+6*(index==2));
    GLCD.GotoXY(15,49);
    GLCD.print("Armour");
    
    GLCD.DrawRect(70-3*(index==3),47-3*(index==3), 45+6*(index==3),11+6*(index==3));
    GLCD.GotoXY(75,49);
    GLCD.print("Medics");
  }
  
  if(buttonPressed == scrollLeft)
   {
     index--;
     if(index < 0)  
       index = 3;
     show_equips(noButton);
   }
   
   if(buttonPressed == scrollRight)
   {
     index++;
     if(index > 3)  
       index = 0;
     show_equips(noButton);
   }
   
   if(buttonPressed == reloadPin)
   {
     Screen = Screen*10 + index;
     lcd_update(Screen);
   }
}

void Sammo(int buttonPressed)
{
  static int index = 0;
  
  if(buttonPressed == noButton)
  {
    GLCD.ClearScreen();
  
    GLCD.GotoXY(40,0);
    GLCD.print("Buy Ammo");
    
    print_rupee(85,15);
    GLCD.GotoXY(92,15);
    GLCD.print(Player.Money);
    
   
    if(index < 3) 
      GLCD.DrawRect(3,27+12*index,80,11);
    else
      GLCD.DrawRect(3,51,80,11);
          
    if(index<3)
    {
      GLCD.FillRect(5,31,2,2);
      GLCD.GotoXY( 14,29);
      GLCD.print(gun[Player.Equips.My_Guns[1].code].name);
      if(Player.Equips.no_of_guns > 2)
      {
        GLCD.FillRect(5,43,2,2);
        GLCD.GotoXY( 14,41);
        GLCD.print(gun[Player.Equips.My_Guns[2].code].name);
      }
      if(Player.Equips.no_of_guns > 3)
      {
        GLCD.FillRect(5,55,2,2);
        GLCD.GotoXY( 14,53);
        GLCD.print(gun[Player.Equips.My_Guns[3].code].name);
      }
    }
    else
    {
      GLCD.FillRect(5,31,2,2);
      GLCD.FillRect(5,43,2,2);
      GLCD.FillRect(5,55,2,2);
      GLCD.GotoXY( 14,29);
      GLCD.print(gun[Player.Equips.My_Guns[index%3 + 2].code].name);
      GLCD.GotoXY( 14,41);
      GLCD.print(gun[Player.Equips.My_Guns[index%3 + 3].code].name);
      GLCD.GotoXY( 14,53);
      GLCD.print(gun[Player.Equips.My_Guns[index%3 + 4].code].name);
    }
  }
  
  if(buttonPressed == scrollLeft)
  {
    index = (index + Player.Equips.no_of_guns - 2) % (Player.Equips.no_of_guns - 1);
    Sammo(noButton);
  }
  
  if(buttonPressed == scrollRight)
  {
    index = (index + 1) % (Player.Equips.no_of_guns);
    Sammo(noButton);
  }
  
  if(buttonPressed == reloadPin)
  {}
  
}

void Sweapon(int buttonPressed)
{
  static int index = 0;
  
  if(buttonPressed == noButton)
  {
    GLCD.ClearScreen();
  
    GLCD.GotoXY(25,0);
    GLCD.print(gun[index+1].name);
    GLCD.GotoXY(65,0);
    GLCD.print("(");
    print_rupee(70,0);
    GLCD.GotoXY(79,0);
    GLCD.print(gun[index+1].cost);
    GLCD.print(")");
    
    GLCD.SetDot(120,0,BLACK);
    GLCD.SetDot(121,1,BLACK);
    GLCD.SetDot(122,2,BLACK);
    GLCD.SetDot(121,3,BLACK);
    GLCD.SetDot(120,4,BLACK);
      
    GLCD.SetDot(7,0,BLACK);
    GLCD.SetDot(6,1,BLACK);
    GLCD.SetDot(5,2,BLACK);
    GLCD.SetDot(6,3,BLACK);
    GLCD.SetDot(7,4,BLACK);
  
    GLCD.GotoXY(0,11);
    GLCD.print(gun[index+1].full_ammo);
    GLCD.print("|");
    GLCD.print(gun[index+1].full_ammo);
    GLCD.GotoXY(33,11);
    GLCD.print("of");
    GLCD.GotoXY(48,11);
    GLCD.print(gun[index+1].total_ammo);
    GLCD.GotoXY(70,11);
    GLCD.print("Load:");
    GLCD.GotoXY(102,11);
    GLCD.print(gun[index+1].reload_time);
    
    GLCD.GotoXY( 2,22);
    GLCD.print("Mode ");    
    GLCD.GotoXY( 50,22);
    GLCD.print(gun[index+1].mode[0].short_name);
    if(gun[index+1].no_of_modes > 1)
    {
      GLCD.GotoXY( 80,22);
      GLCD.print(gun[index+1].mode[1].short_name);
    }
    if(gun[index+1].no_of_modes > 2)
    {
      GLCD.GotoXY( 110,22);
      GLCD.print(gun[index+1].mode[2].short_name);
    }
    
    GLCD.GotoXY( 2,33);
    GLCD.print("Damage ");
    GLCD.GotoXY( 50,33);
    GLCD.print(gun[index+1].mode[0].Xhealth);
    if(gun[index+1].no_of_modes > 1)
    {
      GLCD.GotoXY( 80,33);
      GLCD.print(gun[index+1].mode[1].Xhealth); 
    }
    if(gun[index+1].no_of_modes > 2)
    {
      GLCD.GotoXY( 110,33);
      GLCD.print(gun[index+1].mode[2].Xhealth);
    }
    
    GLCD.GotoXY( 2,44);
    GLCD.print("XAmmo");
    GLCD.GotoXY( 50,44);
    GLCD.print(gun[index+1].mode[0].Xammo);
    if(gun[index+1].no_of_modes > 1)
    {
      GLCD.GotoXY( 80,44);
      GLCD.print(gun[index+1].mode[1].Xammo); 
    }
    if(gun[index+1].no_of_modes > 2)
    {
      GLCD.GotoXY( 110,44);
      GLCD.print(gun[index+1].mode[2].Xammo);
    }
    
    GLCD.GotoXY( 2,55);
    GLCD.print("RT");
    GLCD.GotoXY( 50,55);
    GLCD.print(abs(gun[index+1].mode[0].recoil_time/1000));
    if(gun[index+1].no_of_modes > 1)
    {
      GLCD.GotoXY( 80,55);
      GLCD.print(abs(gun[index+1].mode[1].recoil_time/1000));
    }
    if(gun[index+1].no_of_modes > 2)
    {
      GLCD.GotoXY( 110,55);
      GLCD.print(abs(gun[index+1].mode[2].recoil_time/1000));
    }    
  }
  
  if(buttonPressed == scrollLeft)
  {
    int i;
    index = (index+4)%5;
    for(i = 0; i < Player.Equips.no_of_guns; i++)
      if(Player.Equips.My_Guns[i].code == index)
        break;
    if(i < Player.Equips.no_of_guns)
      Sweapon(scrollLeft);
    else
      Sweapon(noButton);
  }
  
  if(buttonPressed == scrollRight)
  {
    int i;
    index = (index+1)%5;
    for(i = 0; i < Player.Equips.no_of_guns; i++)
      if(Player.Equips.My_Guns[i].code == index)
        break;
    if(i < Player.Equips.no_of_guns)
      Sweapon(scrollRight);
    else
      Sweapon(noButton);
  }
  
  if(buttonPressed == reloadPin)
  {
    if(gun[index+1].cost < Player.Money)
    {
      Player.Money -= gun[index+1].cost;
      Player.Equips.My_Guns[Player.Equips.no_of_guns] = gun[index+1];
      Player.Equips.no_of_guns++;
      Notify("New Gun Bought");
    }
    else
      Notify("Not Enough Money!");
      
    Screen = Home;
    lcd_update(Screen);
  }
    
}

void Sarmour(int buttonPressed)
{
  static int index = 0;
  
  if(buttonPressed == noButton)
  {
    GLCD.ClearScreen();
  
    GLCD.GotoXY(45,0);
    GLCD.print("Armour");
    
    GLCD.SetDot(120,0,BLACK);
    GLCD.SetDot(121,1,BLACK);
    GLCD.SetDot(122,2,BLACK);
    GLCD.SetDot(121,3,BLACK);
    GLCD.SetDot(120,4,BLACK);    
    
    GLCD.SetDot(7,0,BLACK);
    GLCD.SetDot(6,1,BLACK);
    GLCD.SetDot(5,2,BLACK);
    GLCD.SetDot(6,3,BLACK);
    GLCD.SetDot(7,4,BLACK);
    
    GLCD.GotoXY( 8,15);
    GLCD.print(vest[index+1].name);
    
    GLCD.GotoXY( 8,27);
    GLCD.print("Cost:");
    print_rupee(40,27);
    GLCD.GotoXY(49,27);
    GLCD.print(vest[index+1].cost);
    
    GLCD.GotoXY( 14,40);
    GLCD.print("Defense: ");  
    GLCD.print(vest[index+1].defence_factor);  
    
    GLCD.GotoXY( 14,50);
    GLCD.print("Endurance: "); 
    GLCD.print(vest[index+1].decay_rate);   
  }
  
  if(buttonPressed == scrollLeft)
  {
    index = (index+2)%3;
    if(Player.Equips.My_Vest.code == index)
      Sweapon(scrollLeft);
    else
      Sweapon(noButton);
  }
  
  if(buttonPressed == scrollRight)
  {
    index = (index+1)%3;
    if(Player.Equips.My_Vest.code == index)
      Sweapon(scrollRight);
    else
      Sweapon(noButton);
  }
  
  if(buttonPressed == reloadPin)
  {
    if(vest[index+1].cost < Player.Money)
    {
      Player.Money -= vest[index+1].cost;
      Player.Equips.My_Vest = vest[index+1];
      Notify("New Armour Bought");
    }
    else
      Notify("Not Enough Money!");
      
    Screen = Home;
    lcd_update(Screen);
  }
  
}

void Shelmet(int buttonPressed)
{
  static int index = 0;
  
  if(buttonPressed == noButton)
  {
    GLCD.ClearScreen();
  
    GLCD.GotoXY(45,0);
    GLCD.print("Helmet");
    
    GLCD.SetDot(120,0,BLACK);
    GLCD.SetDot(121,1,BLACK);
    GLCD.SetDot(122,2,BLACK);
    GLCD.SetDot(121,3,BLACK);
    GLCD.SetDot(120,4,BLACK);
    
    
    GLCD.SetDot(7,0,BLACK);
    GLCD.SetDot(6,1,BLACK);
    GLCD.SetDot(5,2,BLACK);
    GLCD.SetDot(6,3,BLACK);
    GLCD.SetDot(7,4,BLACK);
    
    GLCD.GotoXY( 8,15);
    GLCD.print(helmet[index+1].name);
    
    GLCD.GotoXY( 8,27);
    GLCD.print("Cost:");
    print_rupee(40,27);
    GLCD.GotoXY(49,27);
    GLCD.print(helmet[index+1].cost);
    
    GLCD.GotoXY( 14,40);
    GLCD.print("Defense: ");
    GLCD.print(helmet[index+1].defence_factor);
    
    GLCD.GotoXY( 14,50);
    GLCD.print("Endurance: ");
    GLCD.print(helmet[index+1].decay_rate);   
  }
  
  if(buttonPressed == scrollLeft)
  {
    index = (index+2)%3;
    if(Player.Equips.My_Helmet.code == index)
      Sweapon(scrollLeft);
    else
      Sweapon(noButton);
  }
  
  if(buttonPressed == scrollRight)
  {
    index = (index+1)%3;
    if(Player.Equips.My_Helmet.code == index)
      Sweapon(scrollRight);
    else
      Sweapon(noButton);
  }
  
  if(buttonPressed == reloadPin)
  {
    if(helmet[index+1].cost < Player.Money)
    {
      Player.Money -= helmet[index+1].cost;
      Player.Equips.My_Helmet = helmet[index+1];
      Notify("New Helmet Bought");
    }
    else
      Notify("Not Enough Money!");
      
    Screen = Home;
    lcd_update(Screen);
  }
  
}

void Smedics(int buttonPressed)
{
  if(buttonPressed == noButton)
  {
    GLCD.ClearScreen();
  
    GLCD.GotoXY(45,0);
    GLCD.print("Medics");
    
    print_rupee(92, 10);
    GLCD.GotoXY(100,10);
    GLCD.print(Player.Money);
    
    GLCD.GotoXY( 8,25);
    GLCD.print("Cost:");
    print_rupee(40,25);
    GLCD.GotoXY(49,25);
    GLCD.print(medics.cost);
    
    GLCD.GotoXY( 8,42);
    GLCD.print("Health + : ");
    GLCD.print(medics.health_factor);
  }
}

void Eprofile(int buttonPressed){}

void Eweapon(int buttonPressed)
{
  static int index = 0;
  
  if(buttonPressed == noButton)
  {
    GLCD.ClearScreen();
  
    GLCD.GotoXY(45,0);
    GLCD.print(Player.Equips.My_Guns[index].name);
    
    GLCD.SetDot(120,0,BLACK);
    GLCD.SetDot(121,1,BLACK);
    GLCD.SetDot(122,2,BLACK);
    GLCD.SetDot(121,3,BLACK);
    GLCD.SetDot(120,4,BLACK);    
    
    GLCD.SetDot(7,0,BLACK);
    GLCD.SetDot(6,1,BLACK);
    GLCD.SetDot(5,2,BLACK);
    GLCD.SetDot(6,3,BLACK);
    GLCD.SetDot(7,4,BLACK);    
  
    GLCD.GotoXY( 7,11);
    GLCD.print("Mag Size: ");
    GLCD.print(Player.Equips.My_Guns[index].full_ammo);
       
    GLCD.GotoXY( 7,22);
    GLCD.print("Reload Time: ");
    GLCD.print((float)Player.Equips.My_Guns[index].reload_time/1000);
    GLCD.print("s");
       
    GLCD.GotoXY( 7,33);
    GLCD.print("Total Ammo: ");
    GLCD.print(Player.Equips.My_Guns[index].total_ammo);
        
    GLCD.GotoXY( 25,44);
    GLCD.print("Modes Available");
   
    //GLCD.DrawRect(11 + i*40,53,35,10);
   
    GLCD.GotoXY( 54,55);
    GLCD.print(Player.Equips.My_Guns[index].mode[0].short_name);
    
    if(Player.Equips.My_Guns[index].no_of_modes > 1)
    {
      GLCD.GotoXY( 14,55);
      GLCD.print(Player.Equips.My_Guns[index].mode[1].short_name);
    }
    
    if(Player.Equips.My_Guns[index].no_of_modes > 2)
    {
      GLCD.GotoXY( 94,55);
      GLCD.print(Player.Equips.My_Guns[index].mode[2].short_name); 
    }  
  }
  
  if(buttonPressed == scrollLeft)
  {
    index = (index + Player.Equips.no_of_guns - 1) % Player.Equips.no_of_guns;
    Eweapon(noButton);
  }
  
  if(buttonPressed == scrollRight)
  {
    index = (index + 1) % Player.Equips.no_of_guns;
    Eweapon(noButton);
  }
}

void Earmour(int buttonPressed)
{
  static int index = 0;
  
  if(buttonPressed == noButton)
  {
    GLCD.ClearScreen();
  
    GLCD.GotoXY(45,0);
    if(index == 0)
      GLCD.print("Armour");
    else
      GLCD.print("Helmet");    
    
    GLCD.SetDot(120,0,BLACK);
    GLCD.SetDot(121,1,BLACK);
    GLCD.SetDot(122,2,BLACK);
    GLCD.SetDot(121,3,BLACK);
    GLCD.SetDot(120,4,BLACK);    
    
    GLCD.SetDot(7,0,BLACK);
    GLCD.SetDot(6,1,BLACK);
    GLCD.SetDot(5,2,BLACK);
    GLCD.SetDot(6,3,BLACK);
    GLCD.SetDot(7,4,BLACK);
    
    GLCD.GotoXY( 8,18);
    if(index == 0)
      GLCD.print(Player.Equips.My_Vest.name);
    else
      GLCD.print(Player.Equips.My_Helmet.name);
    
    GLCD.GotoXY( 14,34);
    GLCD.print("Defense: "); 
    if(index == 0)
      GLCD.print(Player.Equips.My_Vest.defence_factor);
    else
      GLCD.print(Player.Equips.My_Helmet.defence_factor);   
    
    GLCD.GotoXY( 14,45);
    GLCD.print("Endurance: ");
    if(index == 0)
      GLCD.print(Player.Equips.My_Vest.decay_rate);
    else
      GLCD.print(Player.Equips.My_Helmet.decay_rate);
  }
  
  if(buttonPressed == scrollLeft)
  {
    index = !index;
    Earmour(noButton);
  }
  
  if(buttonPressed == scrollRight)
  {
    index = !index;
    Earmour(noButton);
  }
  
  if(buttonPressed == reloadPin)
  {
    Screen = Home;
    lcd_update(Screen);
  }
}

void Emedics(int buttonPressed)
{
  if(buttonPressed == noButton)
  {
    GLCD.ClearScreen();
    
    print_heart(92, 10);
    GLCD.GotoXY(100,10);
    GLCD.print(Player.Health);
    
    GLCD.GotoXY(45,0);
    GLCD.print("Medics");  
    
    GLCD.GotoXY( 8,20);
    GLCD.print("Health + : ");
    GLCD.print(Player.Equips.medics.health_factor);  
    
    GLCD.GotoXY( 8,35);
    GLCD.print("Available: ");
    GLCD.print(Player.Equips.medics.no_available);
    
    if(Player.Equips.medics.no_available > 0)
    {
      GLCD.DrawRect(40,50,40,12);
      GLCD.GotoXY(44,53);
      GLCD.print("Apply");
    }
  }
  
  if((buttonPressed == reloadPin) && (Player.Equips.medics.no_available > 0))
  {
    Player.Equips.medics.no_available--;
    Player.Health += Player.Equips.medics.health_factor;
    if(Player.Health > 100)
      Player.Health = 100;
    Emedics(noButton);
  }  
}

void Notify(char msg[])
{
  static char msg_[20];
  NOTIFY = 1;
  time = millis();
  if(msg != NULL)
  {
    strcpy(msg_, msg);
    lcd_update(Screen);
  }
  if((Screen == Home)||(Screen == gunUpdate)||(Screen == modeUpdate))
  {
    GLCD.GotoXY(72, 45);
    GLCD.print(msg_);
  }
}

//Ammo  Profile  Weapon

