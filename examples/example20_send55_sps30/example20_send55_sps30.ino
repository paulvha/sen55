/*  This example will require BOTH an SPS30 and a SEN55.
 *  
 *  It will read both and display the information measured next to each other so you 
 *  can compare the results.
 *   
 *  As BOTH use the same fixed I2C address (0x69) you need to connect the SEN55 to I2C 
 *  and the SPS30 serial. 
 *  
 *  Version 1.0 / October 2024 / Paulvha
 *  -initial version
 *
 *  December 2024 / paulvha
 *  - update wiring
 *   ..........................................................
 *  Successfully tested on UNO R4
 * 
 *  //////////////// SEN55 //////////////////////
 *  SEN55 Pinout (back / sideview)
 *  
 *  ---------------------
 *  ! 1 2 3 4 5 6        |
 *  !___________         |
 *              \        |  
 *               |       |
 *               """""""""
 *  Wire
 *  SEN55 pin     UNO R4
 *  1 VCC -------- 5V
 *  2 GND -------- GND
 *  3 SDA -------- SDA
 *  4 SCL -------- SCL
 *  5 Select ----- GND  (select I2c)
 *  6 NC
 *  
 *  The pull-up resistors to 5V.
 *  
 *  //////////////// SPS30 //////////////////////
 *  
 *  SPS30 Pinout (backview) SERIAL !!
 *  ----------------------
 *  |                    |
 *  |          1 2 3 4 5 |
 *  ----------------------
 *    
 *  SPS30 pin     UNO R4
 *  1 VCC -------- 5V
 *  2 RX  -------- TX
 *  3 TX  -------- RX 
 *  4 Select      (NOT CONNECTED)
 *  5 GND -------- G
 *  
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
// define serial communication channel to use for SPS30
/////////////////////////////////////////////////////////////
#define SP30_COMMS Serial1

/////////////////////////////////////////////////////////////
/* define Library debug
 * 0 : no messages
 * 1 : request debug messages */
//////////////////////////////////////////////////////////////
#define SEN55_DEBUG 0
#define SPS30_DEBUG 0

///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////
#include "sen55.h"
#include "sps30.h"

// create constructors
SPS30 sps30;
SEN55 sen55;

// variables
struct sen_values_pm sen55_val;
struct sps_values sps30_val;
bool header = true;

void setup() {
  
  Serial.begin(115200);
  while (!Serial) delay(100);

  serialTrigger((char *) "Example20: SEN55 and SPS30. Press <enter> to start");

  Serial.println(F("Trying to connect."));

  // set driver debug level
  sen55.EnableDebugging(SEN55_DEBUG);
  sps30.EnableDebugging(SPS30_DEBUG);

  /////////////////////////////// SEN55 /////////////////////////////////////
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

/////////////////////////////// sps30 /////////////////////////////////////
  SP30_COMMS.begin(115200);

  // Initialize SPS30 library
  if (! sps30.begin(&SP30_COMMS))
  {
    Serial.println(F("Could not set SPS30 serial communication channel. Freeze"));
    while(1);
  }

  // check for SPS30 connection
  if (! sps30.probe()){
    Serial.println(F("Could not probe / connect with SPS30. Freeze"));
    while(1);
  }
  else  Serial.println(F("Detected SPS30."));

  // reset SPS30 connection
  if (! sps30.reset()){
    Serial.println(F("Could NOT reset SPS30. Freeze"));
    while(1);
  }
  
  // start measurement
  if (sps30.start()) Serial.println(F("SPS30 measurement started"));
  else {
    Serial.println(F("Could NOT start SPS30 measurement. Freeze"));
    while(1);
  }

  serialTrigger((char *) "Hit <enter> to start reading.");

  Serial.println(F("Hit <enter> to perform a fan clean."));
}

void loop() {
  
  // start reading
  if (sen55.GetValuesPM(&sen55_val) != SEN55_ERR_OK) {
    Serial.println(F("Could not read SEN55 values."));
    return;
  }

  if (sps30.GetValues(&sps30_val) != SPS30_ERR_OK) {
    Serial.println(F("Could not read SPS30 values."));
    return;
  }

  Display_data();

  // trigger clean cycle (pressed <enter>)
  if (Serial.available()) {
    flush();
    
    Serial.print(F("Clean cycle in progress"));
    sps30.clean();
    Clean_sen55();
    Serial.println(F("\nContinue measurement"));
    
    header = true;
  }
  else
    delay(2000);
}

/**
 * start cleaning and wait for ready clean
 */
void Clean_sen55()
{
  uint8_t stat;
  
  if( !sen55.clean()){
    Serial.println(F("Could not start cleaning Sen55"));
    return;
  }
  
  delay(1000);
  
  do {
    if (sen55.GetStatusReg(&stat) == SEN55_ERR_OK){
      Serial.print(".");
      delay(1000);
    }
  
    else {
      Check_error(); 
    }

  } while (stat == STATUS_FAN_CLEAN_ACTIVE_55);

  Serial.println();
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


/**
 * Display the SPS30 and the SEN55 result next to each other.
 */
void Display_data()
{

  // print header
  if (header) {
    Serial.println(F("\t-------------Mass -----------    ------------- Number --------------   -Average-"));
    Serial.println(F("\t     Concentration [μg/m3]             Concentration [#/cm3]             [μm]"));
    Serial.println(F("\tP1.0\tP2.5\tP4.0\tP10\tP0.5\tP1.0\tP2.5\tP4.0\tP10\tPartSize\n"));
    header = false;
  }
  Serial.print(F("SEN5x\t"));
  Serial.print(sen55_val.MassPM1);
  Serial.print(F("\t"));
  Serial.print(sen55_val.MassPM2);
  Serial.print(F("\t"));
  Serial.print(sen55_val.MassPM4);
  Serial.print(F("\t"));
  Serial.print(sen55_val.MassPM10);
  Serial.print(F("\t"));
  Serial.print(sen55_val.NumPM0);
  Serial.print(F("\t"));
  Serial.print(sen55_val.NumPM1);
  Serial.print(F("\t"));
  Serial.print(sen55_val.NumPM2);
  Serial.print(F("\t"));
  Serial.print(sen55_val.NumPM4);
  Serial.print(F("\t"));
  Serial.print(sen55_val.NumPM10);
  Serial.print(F("\t"));
  Serial.print(sen55_val.PartSize);
  Serial.print(F("\n"));

  Serial.print(F("SPS30\t"));
  Serial.print(sps30_val.MassPM1);
  Serial.print(F("\t"));
  Serial.print(sps30_val.MassPM2);
  Serial.print(F("\t"));
  Serial.print(sps30_val.MassPM4);
  Serial.print(F("\t"));
  Serial.print(sps30_val.MassPM10);
  Serial.print(F("\t"));
  Serial.print(sps30_val.NumPM0);
  Serial.print(F("\t"));
  Serial.print(sps30_val.NumPM1);
  Serial.print(F("\t"));
  Serial.print(sps30_val.NumPM2);
  Serial.print(F("\t"));
  Serial.print(sps30_val.NumPM4);
  Serial.print(F("\t"));
  Serial.print(sps30_val.NumPM10);
  Serial.print(F("\t"));
  Serial.print(sps30_val.PartSize);
  Serial.println(F("\n"));
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
}
