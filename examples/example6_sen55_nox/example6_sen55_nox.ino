/*  
 *  version 1.0 / october 2024 / paulvha
 * 
 *  This example will read the MASS / NOX and VOC values and save/restore NOX algorithm.
 *  
 *  NOX values  (source datasheet SEN55)
 *  The NOx algorithm can be customized by tuning 6 different parameters. More details 
 *  on the tuning instructions are provided in the application note “Engineering Guidelines for SEN5x”. 
 *  This command is available only in idle mode. In measure mode, this command has no effect. 
 *  In addition, it has no effect if at least one parameter is outside the speciﬁed range.
 *  Both are handled/checked by the library.
 *  
 *  Pressing enter during the measurement will display the current NOX-values and provide a menu to
 *  change the parameters that can be changed. 
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
// wait max 10 seconds on input
/////////////////////////////////////////////////////////////
#define INPUTDELAY 10

/////////////////////////////////////////////////////////////
/* define library debug
 * 0 : no messages
 * 1 : request debug messages */
 //////////////////////////////////////////////////////////////
#define DEBUG 1

///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////
#include "sen55.h"

SEN55 sen55;

struct sen_values val;
struct sen_xox nox;

void setup() {
  
  Serial.begin(115200);
  while (!Serial) delay(100);

  serialTrigger((char *) "SEN55-Example6: Display basic values and get/set Nox Algorithm. press <enter> to start");

  Serial.println(F("Trying to connect."));

  // set library debug level
  sen55.EnableDebugging(DEBUG);

  Wire.begin();
  
  // Begin communication channel;
  if (! sen55.begin(&Wire)) {
    Serial.println("could not initialize communication channel.");
    while(1);
  }

  // check for SEN55 connection
  if (! sen55.probe()) {
    Serial.println("could not probe / connect with SEN55.");
    while(1);
  }
  else  {
    Serial.println(F("Detected SEN5x."));
  }

  Display_Device_info();

  Serial.setTimeout(INPUTDELAY * 1000);
}

void loop() {
  
  // start reading
  if (sen55.GetValues(&val) != SEN55_ERR_OK) {
    Serial.println("Could not read values.");
    return;
  }
  else
    Display_val();

  // trigger reading adjusting Nox algorithm (pressed <enter>)
  if (Serial.available()) {
    flush();
    CheckNoxValues();
  }
  else
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


void CheckNoxValues() {
  int inp;
  
  // Nox can only be changed when idle
  // (re)start will happen with the next measurement
  if (! sen55.stop()) {
    Serial.println("Could not Stop measurement.");
    return;
  }

  // give time to stop
  delay(1000);
  
  if(sen55.GetNoxAlgorithm(&nox) != SEN55_ERR_OK){
    Serial.println("Could not read Nox Algorithm.");
    return; 
  }
  
  while(1){
    
    disp_nox_info();

    inp = GetInput(1,9);

    // cancel (or timeout)
    if (inp == 7 || inp == -100) {
      Serial.println("Cancel");
      return;
    }
    
    Serial.println(inp);
    
    if (inp == 3 || inp == 5) {
      Serial.println("Can not change this option. Info only");
      continue;
    }

    // update Nox algorithm
    if (inp == 9) {
    
      // write back
      if (sen55.SetNoxAlgorithm(&nox) != SEN55_ERR_OK)
        Serial.println(F("could not update Nox algorithm."));
      else
        Serial.println(F("updated Nox algorithm"));
    
      return;
    }

    if (inp == 1) {
      Serial.print(F("1 IndexOffset\t\t"));
      Serial.print(nox.IndexOffset);
      Serial.println(" range 1..250");
      inp = GetInput(1,250);
      if (inp == -100) continue;
      nox.IndexOffset = inp;
    }
    
    else if (inp == 2) {
      Serial.print(F("2 LearnTimeOffsetHours   "));
      Serial.print(nox.LearnTimeOffsetHours);
      Serial.println(" range 1..1000");
      inp = GetInput(1,1000);
      if (inp == -100) continue;
      nox.LearnTimeOffsetHours = inp;
    }

    else if (inp == 4) {
      Serial.print(F("4 GateMaxDurationMin   "));
      Serial.print(nox.GateMaxDurationMin);
      Serial.println(" range 0..3000");
      inp = GetInput(0,3000);
      if (inp == -100) continue;
      nox.GateMaxDurationMin = inp;
    }

    else if (inp == 6) {
      Serial.print(F("6 GainFactor   "));
      Serial.print(nox.GainFactor);
      Serial.println(" range 1..1000");
      inp = GetInput(1,1000);
      if (inp == -100) continue;
      nox.GainFactor = inp;
    }
  }
}

void disp_nox_info()
{
  Serial.println(F("\n************************\n"));
  Serial.print(F("1 IndexOffset\t\t"));
  Serial.println(nox.IndexOffset);
  Serial.print(F("2 LearnTimeOffsetHours  "));
  Serial.println(nox.LearnTimeOffsetHours);
  Serial.print(F("3 LearnTimeGainHours    "));
  Serial.println(nox.LearnTimeGainHours);
  Serial.print(F("4 GateMaxDurationMin    "));
  Serial.println(nox.GateMaxDurationMin);
  Serial.print(F("5 stdInitial\t\t"));
  Serial.println(nox.stdInitial);
  Serial.print(F("6 GainFactor\t\t"));
  Serial.println(nox.GainFactor);
  
  Serial.println("7 Cancel");
  Serial.println("9 update Sen55\n");
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
  
  Serial.print("Provide the entry-number to change. Only <enter> is return. (or wait ");
  Serial.print(INPUTDELAY);
  Serial.println(" seconds)");
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
