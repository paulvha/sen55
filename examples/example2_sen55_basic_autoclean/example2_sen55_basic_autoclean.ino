
/*  
 *  version 1.0 / october 2024 / paulvha
 *    
 *  This example will connect to the SEN55. It will read the serialnumber, name and different software
 *  levels. 
 *  
 *  It will display the Mass, VOC, NOC, Temperature and humidity information, as well as display
 *  Mass and PM numbers. 
 *  
 *  press <enter> during measurement to allow to perform an fan clean now or get / set a new 
 *  interval for autoclean.
 *  
 *  Tested on UNOR4, Artemis ATP, UNOR3, ATmega, Due, ESP32
 
 *  December 2024 / paulvha
 *  - update wiring
 *   ..........................................................
 *  SEN55 Pinout (back  sideview)
 *  ---------------------
 *  ! 1 2 3 4 5 6        |
 *  !___________         |
 *              \        |  
 *               |       |
 *               """""""""
 *  .........................................................
 *  Successfully tested on ESP32
 *
 *  SEN55 pin     ESP32
 *  1 VCC -------- VUSB
 *  2 GND -------- GND
 *  3 SDA -------- SDA (pin 21)
 *  4 SCL -------- SCL (pin 22)
 *  5 Select ----- GND (select I2c)
 *  6 NOT used/connected
 *
 *  The pull-up resistors should be to 3V3
 *  ..........................................................
 *  
 *  Successfully tested on ATMEGA2560, Due
 *
 *  SEN55 pin     ATMEGA
 *  1 VCC -------- 5V
 *  2 GND -------- GND
 *  3 SDA -------- SDA
 *  4 SCL -------- SCL
 *  5 Select ----- GND  (select I2c)
 *  6 NOT used/connected
 *
 *  ..........................................................
 *  Successfully tested on UNO R3
 *
 *  SEN55 pin     UNO
 *  1 VCC -------- 5V
 *  2 GND -------- GND
 *  3 SDA -------- A4
 *  4 SCL -------- A5
 *  5 Select ----- GND  (select I2c)
 *  6 NOT used/connected
 *
 *  When UNO-board is detected some buffers reduced and the call 
 *  to GetErrDescription() is removed to allow enough memory.
 *  
 *  ..........................................................
 *  Successfully tested on UNO R4  & Artemis/Apollo3 Sparkfun
 *
 *  SEN55 pin     UNO R4
 *  1 VCC -------- 5V
 *  2 GND -------- GND
 *  3 SDA -------- SDA
 *  4 SCL -------- SCL
 *  5 Select ----- GND  (select I2c)
 *  6 NOT used/connected
 *  
 *  The pull-up resistors should be to 5V.
 * 
 *  ================================ Disclaimer ======================================
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  ===================================================================================
 *
 *  NO support, delivered as is, have fun, good luck !!
 *  
 */

/////////////////////////////////////////////////////////////
// wait max 10 seconds on input from keyboard
/////////////////////////////////////////////////////////////
#define INPUTDELAY 10

/////////////////////////////////////////////////////////////
/* define driver debug
 * 0 : no messages
 * 1 : request debug messages */
 //////////////////////////////////////////////////////////////
#define DEBUG 0

///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////

#include "sen55.h"

SEN55 sen55;

struct sen_values val;
struct sen_values_pm valPM;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);

  serialTrigger((char *) "SEN55-Example2: Display basic values & auto clean. press <enter> to start");

  Serial.println(F("Trying to connect."));

  // set library debug level
  sen55.EnableDebugging(DEBUG);

  Wire.begin();
  
  // Begin communication channel;
  if (! sen55.begin(&Wire)) {
    Serial.println(F("could not initialize communication channel."));
    while(1);
  }

  // check for SEN55 connection
  if (! sen55.probe()) {
    Serial.println(F("could not probe / connect with SEN55."));
    while(1);
  }
  else  {
    Serial.println(F("Detected SEN5x."));
  }

  // reset SEN55
  if (! sen55.reset()) {
    Serial.println(F("could not reset SEN55."));
    while(1);
  }
  
  Serial.setTimeout(INPUTDELAY * 1000);
}

void loop() {

  // start reading
  if (sen55.GetValues(&val) != SEN55_ERR_OK) {
    Serial.println(F("Could not read values."));
    return;
  }
  else
    Display_val();

  if (sen55.GetValuesPM(&valPM) != SEN55_ERR_OK) {
    Serial.println(F("Could not read PM values."));
    return;
  }
  else
    Display_valPM();
    
  // trigger clean cycle (pressed <enter>)
  if (Serial.available()) ActClean();
  else delay(2000);

  // check clean in progress and wait until ready
  Check_Clean_sen55();
}

/**
 * set fan cleaning
 */
void ActClean()
{
  int inp;
  
  while (1) {
    Serial.println(F("1. perform clean now"));
    Serial.println(F("2. get/set autoclean"));
    Serial.println(F("3. cancel/return"));

    inp = GetInput(1,3);
   
    // cancel (or timeout)
    if (inp == 3 || inp == -100) {
      Serial.println(F("Cancel"));
      return;
    }

    Serial.println(inp);

    if (inp == 1) {
      if( !sen55.clean()){
        Serial.println(F("Could not start cleaning Sen55"));
      }
      delay(1000);
      return;

    }
    else  if (inp == 2) {
      SetAutoClean();
      return;
    }
  }
}
  

/**
 * @brief: Get & Set new Auto Clean Interval
 *
 */
void SetAutoClean()
{
  uint32_t interval;

  // try to get interval
  if (sen55.GetAutoCleanInt(&interval) == SEN55_ERR_OK) {
    Serial.print(F("Current Auto Clean interval: "));
    Serial.print(interval);
    Serial.println(F(" seconds"));
  }
  else{
    Serial.println(F("could not get clean interval."));
    return;
  }

  Serial.println(F("provide new interval range: 0...640800  (0 - disable)"));
  
  int new_interval = GetInput(0, 640800);
  
  if (new_interval == -100) {
    Serial.println(F("Cancel"));
    return;
  }

  // try to set interval
  interval = (uint32_t)new_interval;
  
  if (sen55.SetAutoCleanInt(interval) == SEN55_ERR_OK) {
    Serial.print(F("Auto Clean interval now set : "));
    Serial.print(interval);
    Serial.println(F(" seconds"));
  }
  else {
    Serial.println(F("could not set clean interval."));
    return;
  }
  
  // try to get interval
  if (sen55.GetAutoCleanInt(&interval) == SEN55_ERR_OK) {
    Serial.print(F("Current Auto Clean interval: "));
    Serial.print(interval);
    Serial.println(F(" seconds"));
  }
  else
    Serial.println(F("could not get clean interval."));
}

/**
 * check for cleaning in action and wait for ready.
 */
void Check_Clean_sen55()
{
  uint8_t stat;

  if (sen55.GetStatusReg(&stat) == SEN55_ERR_OK){
   if(stat != STATUS_FAN_CLEAN_ACTIVE_55) return;
  }
  
  Serial.print(F("Clean cycle in progress"));
  
  do {
    if (sen55.GetStatusReg(&stat) == SEN55_ERR_OK){
      Serial.print(".");
      delay(1000);
    }
    else {
      Check_error(); 
    }

  } while (stat == STATUS_FAN_CLEAN_ACTIVE_55);

  Serial.println(F("\nContinue measurement\n"));
}

/**
 * check for status errors
 * 
 * return :
 * true : all OK
 * false : status error
 */
bool Check_error() {
  uint8_t stat;

  if (sen55.GetStatusReg(&stat) == SEN55_ERR_OK) return(true);

  Serial.println("ERROR !!");
  if (stat & STATUS_SPEED_ERROR_55) {
    Serial.println(F("Fan speed is too high or too low"));
  }
  
  if (stat & STATUS_LASER_ERROR_55) {
    Serial.println(F("Laser is switched on and current is out of range"));
  }

  if (stat & STATUS_FAN_ERROR_55) {
    Serial.println(F("Fan is switched on, but the measured fan speed is 0 RPM"));
  }

  if (stat & STATUS_GAS_ERROR_55) {
    Serial.println(F("Gas sensor error"));
  }
  
  if (stat & STATUS_RHT_ERROR_55) {
    Serial.println(F("Error in internal communication with the RHT sensor"));
  }     

  return(false);
}

void Display_val()
{
  Serial.print("MassPM1:\t");
  Serial.println(val.MassPM1);
  Serial.print("MassPM2:\t");
  Serial.println(val.MassPM2);
  Serial.print("MassPM4:\t");
  Serial.println(val.MassPM4);
  Serial.print("MassPM10:\t");
  Serial.println(val.MassPM10); 
  Serial.print("Humidity:\t");
  Serial.println(val.Hum);
  Serial.print("Temperature:\t");
  Serial.println(val.Temp);
  Serial.print("VOC:\t\t");
  Serial.println(val.VOC);
  Serial.print("NOX:\t\t");
  Serial.println(val.NOX);
}

void Display_valPM()
{
  Serial.print("MassPM1:\t");
  Serial.println(valPM.MassPM1);
  Serial.print("MassPM2:\t");
  Serial.println(valPM.MassPM2);
  Serial.print("MassPM4:\t");
  Serial.println(valPM.MassPM4);
  Serial.print("MassPM10:\t");
  Serial.println(valPM.MassPM10); 

  Serial.print("NumPM0.5:\t");
  Serial.println(valPM.NumPM0);
  Serial.print("NumPM1:\t\t");
  Serial.println(valPM.NumPM1);
  Serial.print("MassPM2:\t");
  Serial.println(valPM.NumPM2);
  Serial.print("MassPM4:\t");
  Serial.println(valPM.NumPM4);
  Serial.print("MassPM10:\t");
  Serial.println(valPM.NumPM10); 

  Serial.print("Partsize:\t");
  Serial.println(valPM.PartSize);
  
  Serial.println();
}

/**
 * get input from keyboard
 * @param 
 *  maxop: the maximum number to expect
 *  minop : the minimum number
 * 
 * return : 
 *  -100 NO entry was given
 *  else valid entry.
 */
int GetInput(int minop, int maxop)
{
  // flush pending input
  flush();
  
  Serial.print(F("Provide the entry-number to change. Only <enter> is return. (or wait "));
  Serial.print(INPUTDELAY);
  Serial.println(F(" seconds"));
  while(1) {
    
    String Keyb = Serial.readStringUntil(0x0a);

    if (Keyb.length() < 2) return -100;
    int in = Keyb.toInt();
    
    if(in > maxop || in < minop) {
      Serial.print(in);
      Serial.print(" Invalid option. Min ");
      Serial.print(minop);
      Serial.print(", Max ");
      Serial.println(maxop);
    }
    else
      return(in);
  }
}

/** 
 *  display the device information
 */
void Display_Device_info()
{
  char num[32];
  
  // get SEN55 serial number
  if (sen55.GetSerialNumber(num,32) != SEN55_ERR_OK) {
    Serial.println(F("could not read serial number."));
    while(1);
  }
  Serial.print(F("Serial number: "));
  Serial.println(num);

  //get product name
  if (sen55.GetProductName(num,32) != SEN55_ERR_OK) {
    Serial.println(F("could not read product name."));
    while(1);
  }
  Serial.print(F("Product name: "));
  Serial.println(num);

  Display_Versions();
}

/**
 * Display different versions
 * 
 * This is not well documented in the datasheet
 */
void Display_Versions()
{
  struct sen_version v;
  
  if (sen55.GetVersion(&v) != SEN55_ERR_OK) {
    Serial.println(F("could not read version. Freeze"));
    while(1);
  }

  Serial.println(F("Version info:"));
  Serial.print(F("\tFirmware: "));
  Serial.print(v.F_major);
  Serial.print(".");
  Serial.println(v.F_minor);
  
  Serial.print(F("\tHardware: "));
  Serial.print(v.H_major);
  Serial.print(".");
  Serial.println(v.H_minor);

  Serial.print(F("\tProtocol: "));
  Serial.print(v.P_major);
  Serial.print(".");
  Serial.println(v.P_minor);

  Serial.print(F("\tLibrary:  "));
  Serial.print(v.L_major);
  Serial.print(".");
  Serial.println(v.L_minor);
}


/**
 * flush input
 */
void flush()
{
  do {
    delay(200);
    Serial.read();
  } while(Serial.available());
}

/**
 * serialTrigger prints repeated message, then waits for enter
 * to come in from the serial port.
 */
void serialTrigger(char * mess)
{
  Serial.println();

  while (!Serial.available()) {
    Serial.println(mess);
    delay(2000);
  }

  flush();
}
