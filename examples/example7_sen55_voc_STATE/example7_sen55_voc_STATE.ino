/*  
 *  version 1.0 / october 2024 / paulvha
 * 
 *  This example will read the MASS / NOX and VOC values and save/restore VOC algorithm STATE.
 *  
 *  For setting the VOC algorithm options see example5 
 *  
 *  VOC algorithm STATE values  (source datasheet SEN55: chapter6.1.11)
 *  Allows to backup and restore the VOC algorithm state to resume operation after a short 
 *  interruption, skipping initial learning phase. By default, the VOC algorithm resets 
 *  its state to initial values each time a measurement is started, even if the measurement 
 *  was stopped only for a short time. So, the VOC index output value needs a long time until 
 *  it is stable again. This can be avoided by restoring the previously memorized algorithm 
 *  state before starting the measuremode
 *  
 *  At a regular interval it will read and save the VOC value and write that back at 
 *  'VOC_INTERVAL' after reading. Reading interval is 'VOC_INTERVAL' + random seconds between 1 and 10. 
 *  
 *  There is NO further information available about the VOC algorithm STATE and what this data represents.
 *  
 *  One could extend this to store the read data in EEPROM and restore at start up
 *  
 *  BE AWARE: The datasheet seems to indicate 0 - 10 data bytes. In reality it is only 8.
 * 
 *  Tested on UNOR4, Artemis ATP, UNOR3, ATmega, Due, ESP32
 * 
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
/* define driver debug
 * 0 : no messages
 * 1 : request debug messages */
 //////////////////////////////////////////////////////////////
#define DEBUG 0

//////////////////////////////////////////////////////////////
// how often (in seconds) to read and restore VOC values
//////////////////////////////////////////////////////////////
#define VOC_INTERVAL 10

///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////

#include "sen55.h"

SEN55 sen55;

struct sen_values val;
uint8_t HoldVocInfo[VOC_ALO_SIZE];    // hold save voc STATE
bool ReadVocInfo = false;
unsigned long st;

void setup() {
  
  Serial.begin(115200);
  while (!Serial) delay(100);

  serialTrigger((char *) "SEN55-Example7: Display basic values and get/set VOC STATE press <enter> to start");

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

  Display_Device_info();

  st = millis();
}

void loop() {
  static bool save = true;
  static uint8_t AddNextSave = 0;
  
  // start reading
  if (sen55.GetValues(&val) != SEN55_ERR_OK) {
    Serial.println(F("Could not read values."));
    return;
  }
  else
    Display_val();

  // Trigger VOC reading / restore
  if (millis() - st > (VOC_INTERVAL + AddNextSave) * 1000) {
       
    // set start time 
    st = millis();

    if( Voc_Algorithm_State(save) ) {
      
      // if just restored add time for next save 
      if ( ! save) { 
        AddNextSave = random(1,10);
        Serial.print(F("Next save will happen (sec) "));
        Serial.println(VOC_INTERVAL + AddNextSave);
        save = true;    // save next time
      }
      else {
        save = false;   // restore next time as we just saved
        AddNextSave = 0;
      }
    }
  }
  
  delay(2000);
}

/**
 * display the mass and NOC/VOC values
 */
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

/**
 * read of write VOC algorithm state values
 * 
 * @param save :
 *  true : read 
 *  false : write earlier read data
 *  
 *  @return :
 *  true : sucessfull action
 *  false : failed
 */
bool Voc_Algorithm_State(bool save)
{
  // read the VOC state information
  if (save) {
    if (sen55.GetVocAlgorithmState(HoldVocInfo,VOC_ALO_SIZE) != SEN55_ERR_OK) {
      Serial.println(F("could not READ VOC Stateinformation."));
      return(false);
    }

    // indicate that we have valid data
    ReadVocInfo = true;
    Serial.print(F("VOC State values read: "));
    for (int i =0 ; i < VOC_ALO_SIZE ;i++){
      Serial.print(HoldVocInfo[i],HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  // restore VOC algorithm state (if saved before)
  else if (ReadVocInfo) {
  
    if (sen55.SetVocAlgorithmState(HoldVocInfo,VOC_ALO_SIZE) != SEN55_ERR_OK) {
      Serial.println(F("could not WRITE VOC State information."));
      return(false);    
    }
    
    Serial.print("VOC State values restored ");

    // don't restore twice the same info
    ReadVocInfo = false;
  }
  return(true);
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
