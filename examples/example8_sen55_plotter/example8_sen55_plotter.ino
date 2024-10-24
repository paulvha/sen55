#include "sen55.h"
/*  
 *  version 1.0 / october 2024 / paulvha
 *  
 *  This basic reading example sketch will connect to an SEN55 for getting data and
 *  display the available data. use The plotter : Tools -> Serial Plotter OR Cntrl+Shift+L
 *  It will take about 10 seconds to start (also depending on the SKIPFIRST settings)
 *  You can test with normal serial monitor.
 *   
 *   ..........................................................
 *  SEN55 Pinout (backview)
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
 *  2 SDA -------- SDA (pin 21)
 *  3 SCL -------- SCL (pin 22)
 *  4 Select ----- GND (select I2c)
 *  5 GND -------- GND
 *  6 NOT used/connected
 *
 *  The pull-up resistors should be to 3V3
 *  ..........................................................
 *  
 *  Successfully tested on ATMEGA2560, Due
 *
 *  SEN55 pin     ATMEGA
 *  1 VCC -------- 5V
 *  2 SDA -------- SDA
 *  3 SCL -------- SCL
 *  4 Select ----- GND  (select I2c)
 *  5 GND -------- GND
 *  6 NOT used/connected
 *
 *  ..........................................................
 *  Successfully tested on UNO R3
 *
 *  SEN55 pin     UNO
 *  1 VCC -------- 5V
 *  2 SDA -------- A4
 *  3 SCL -------- A5
 *  4 Select ----- GND  (select I2c)
 *  5 GND -------- GND
 *  6 NOT used/connected
 *
 *  When UNO-board is detected some buffers reduced and the call 
 *  to GetErrDescription() is removed to allow enough memory.
 *  
 *  ..........................................................
 *  Successfully tested on UNO R4
 *
 *  SEN55 pin     UNO R4
 *  1 VCC -------- 5V
 *  2 SDA -------- SDA
 *  3 SCL -------- SCL
 *  4 Select ----- GND  (select I2c)
 *  5 GND -------- GND
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
/* define driver debug
 * 0 : no messages
 * 1 : request debug messages */
 //////////////////////////////////////////////////////////////
#define DEBUG 0

//////////////////////////////////////////////////////////////
/* define what you want to see plotted
 *  MASS 1: include MASS information     0: exclude
 *  NUM  1: include NUM information      0: exclude
 *  AVG  1: include average size         0: exclude
 *
 */
//////////////////////////////////////////////////////////////
#define MASS 1
#define NUM 0
#define AVG 0

//////////////////////////////////////////////////////////////
/* skip first x readings as they might be wrong
 * due to start-up */
//////////////////////////////////////////////////////////////
#define SKIPFIRST 5

// set for plotter output
int plotter = 1
;
///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////

SEN55 sen55;
struct sen_values_pm val;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);

  if (plotter == 1)
    serialTrigger((char *) "start");
  else
    serialTrigger((char *) "SEN55-Example8: Display basic values as plotter. press <enter> to start");

  Serial.println(F("Trying to connect."));

  // set library debug level
  sen55.EnableDebugging(DEBUG);

  Wire.begin();
  
  // Begin communication channel;
  if (! sen55.begin(&Wire)) {
    if (plotter == 1) Serial.println("Error channel");
    else Serial.println("could not initialize communication channel.");
    while(1);
  }

  // check for SEN55 connection
  if (! sen55.probe()) {
    if (plotter == 1) Serial.println("Error probe");
    else Serial.println("could not probe / connect with SEN55.");
    while(1);
  }
  else  {
    Serial.println(F("Detected SEN5x."));
  }

  // reset
  if (! sen55.reset()) {
    if (plotter == 1) Serial.println("Error reset");
    else  Serial.println("could not reset SEN55.");
    while(1);
  }
  
  //Display_Device_info();
}

void loop() {

  read_all();
  delay(2000);
}

/**
 * @brief : read and display all values
 */
bool read_all()
{
  uint8_t ret, error_cnt = 0;
  static uint8_t s_kip = SKIPFIRST;

  // loop to get data
  do {

    ret = sen55.GetValuesPM(&val);

    // data might not have been ready
    if (ret == SEN55_ERR_DATALENGTH) {

      if (error_cnt++ > 3) {
        Serial.print("Error during reading values: ");
        Serial.println(ret);
        return(false);
      }
      delay(1000);
    }

    // if other error
    else if(ret != SEN55_ERR_OK) {
      Serial.print("Error during reading values: ");
      Serial.println(ret);
      Check_error();
      return(false);
    }

  } while (ret != SEN55_ERR_OK);

  // skip first reading
  if (s_kip > 0) {
    s_kip--;
    Serial.print(".");
    return(true);
  }

if (MASS) {
  Serial.print("MassPM1:");
  Serial.print(val.MassPM1);
  Serial.print(",");
  Serial.print("MassPM2:");
  Serial.print(val.MassPM2);
  Serial.print(",");
  Serial.print("MassPM4:");
  Serial.print(val.MassPM4);
  Serial.print(",");
  Serial.print("MassPM10:");
  Serial.println(val.MassPM10);
}

if (NUM) {
  Serial.print("NumPM0:");
  Serial.print(val.NumPM0);
  Serial.print(F("\t"));
  Serial.print(val.NumPM1);
  Serial.print(F("\t"));
  Serial.print(val.NumPM2);
  Serial.print(F("\t"));
  Serial.print(val.NumPM4);
  Serial.print(F("\t"));
  Serial.print(val.NumPM10);
}

if (AVG) {
  Serial.println(val.PartSize);
}
  //Serial.print(F("\n"));

  return(true);
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
    Serial.println("Fan speed is too high or too low");
  }
  
  if (stat & STATUS_LASER_ERROR_55) {
    Serial.println("Laser is switched on and current is out of range");
  }

  if (stat & STATUS_FAN_ERROR_55) {
    Serial.println("Fan is switched on, but the measured fan speed is 0 RPM");
  }

  if (stat & STATUS_GAS_ERROR_55) {
    Serial.println("Gas sensor error");
  }
  
  if (stat & STATUS_RHT_ERROR_55) {
    Serial.println("Error in internal communication with the RHT sensor");
  }     

  return(false);
}
/** 
 *  display the device information
 */
void Display_Device_info()
{
  char num[32];
  
  // get SEN55 serial number
  if (sen55.GetSerialNumber(num,32) != SEN55_ERR_OK) {
    Serial.println("could not read serial number.");
    while(1);
  }
  Serial.print(F("Serial number: "));
  Serial.println(num);

  //get product name
  if (sen55.GetProductName(num,32) != SEN55_ERR_OK) {
    Serial.println("could not read product name.");
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
    Serial.println("could not read version. Freeze");
    while(1);
  }

  Serial.println("Version info :");
  Serial.print("\tFirmware: ");
  Serial.print(v.F_major);
  Serial.print(".");
  Serial.println(v.F_minor);
  
  Serial.print("\tHardware: ");
  Serial.print(v.H_major);
  Serial.print(".");
  Serial.println(v.H_minor);

  Serial.print("\tProtocol: ");
  Serial.print(v.P_major);
  Serial.print(".");
  Serial.println(v.P_minor);

  Serial.print("\tLibrary:  ");
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
