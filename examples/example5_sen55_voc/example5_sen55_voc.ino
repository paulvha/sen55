/*  
 *  version 1.0 / october 2024 / paulvha
 * 
 *  This example will read the MASS / NOX and VOC values and save/restore VOC algorithm.
 *  
 *  VOC values  (source datasheet SEN55)
 *  The VOC algorithm can be customized by tuning 6 different parameters. More details on the tuning 
 *  instructions are provided in the application note “Engineering Guidelines for SEN5x”. 
 *  Note that this command is available only in idle mode. In measure mode, this command has no effect. 
 *  In addition, it has no effect if at least one parameter is outside the speciﬁed range.
 *  Both are checked / handled by the software.
 *  
 *  Pressing enter during the measurement will display the current VOC-values and provide a menu to
 *  change the parameters that can be changed. 
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
// wait max 10 seconds on input
/////////////////////////////////////////////////////////////
#define INPUTDELAY 10

/////////////////////////////////////////////////////////////
/* define library debug
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
struct sen_xox voc;

void setup() {
  
  Serial.begin(115200);
  while (!Serial) delay(100);

  serialTrigger((char *) "SEN55-Example5: Display basic values and get/set VOC Algorithm. press <enter> to start");

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

  // trigger reading adjusting VOC algorithm (pressed <enter>)
  if (Serial.available()) {
    flush();
    CheckVocValues();
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
  Serial.println();
}


void CheckVocValues() {
  int inp;
  
  // VOC can only be changed when idle
  // (re)start will happen with the next measurement
  if (! sen55.stop()) {
    Serial.println(F("Could not Stop measurement."));
    return;
  }

  // give time to stop
  delay(1000);
  
  if(sen55.GetVocAlgorithm(&voc) != SEN55_ERR_OK){
    Serial.println(F("Could not read VOC Algorithm."));
    return; 
  }
  
  while(1){
    
    disp_voc_info();

    inp = GetInput(1,9);

    // cancel (or timeout)
    if (inp == 7 || inp == -100) {
      Serial.println("Cancel");
      return;
    }
    
    Serial.println(inp);
    
    // update Voc algorithm
    if (inp == 9) {
    
      // write back
      if (sen55.SetVocAlgorithm(&voc) != SEN55_ERR_OK)
        Serial.println(F("could not update VOC algorithm."));
      else
        Serial.println(F("updated VOC algorithm"));
    
      return;
    }

    if (inp == 1) {
      Serial.print(F("1 IndexOffset\t\t"));
      Serial.print(voc.IndexOffset);
      Serial.println(" range 1..250");
      inp = GetInput(1,250);
      if (inp == -100) continue;
      voc.IndexOffset = inp;
    }
    
    else if (inp == 2) {
      Serial.print(F("2 LearnTimeOffsetHours   "));
      Serial.print(voc.LearnTimeOffsetHours);
      Serial.println(" range 1..1000");
      inp = GetInput(1,1000);
      if (inp == -100) continue;
      voc.LearnTimeOffsetHours = inp;
    }

    else if (inp == 3) {
      Serial.print(F("3 LearnTimeGainHours   "));
      Serial.print(voc.LearnTimeGainHours);
      Serial.println(" range 1..1000");
      inp = GetInput(1,1000);
      if (inp == -100) continue;
      voc.LearnTimeGainHours = inp;
    }

    else if (inp == 4) {
      Serial.print(F("4 GateMaxDurationMin   "));
      Serial.print(voc.GateMaxDurationMin);
      Serial.println(" range 0..3000");
      inp = GetInput(0,3000);
      if (inp == -100) continue;
      voc.GateMaxDurationMin = inp;
    }

    else if (inp == 5) {
      Serial.print(F("4 stdInitial   "));
      Serial.print(voc.stdInitial);
      Serial.println(" range 10..5000");
      inp = GetInput(10,5000);
      if (inp == -100) continue;
      voc.stdInitial = inp;
    }
    
    else if (inp == 6) {
      Serial.print(F("6 GainFactor   "));
      Serial.print(voc.GainFactor);
      Serial.println(" range 1..1000");
      inp = GetInput(1,1000);
      if (inp == -100) continue;
      voc.GainFactor = inp;
    }
  }
}

void disp_voc_info()
{
  Serial.println(F("\n************************\n"));
  Serial.print(F("1 IndexOffset\t\t"));
  Serial.println(voc.IndexOffset);
  Serial.print(F("2 LearnTimeOffsetHours  "));
  Serial.println(voc.LearnTimeOffsetHours);
  Serial.print(F("3 LearnTimeGainHours    "));
  Serial.println(voc.LearnTimeGainHours);
  Serial.print(F("4 GateMaxDurationMin    "));
  Serial.println(voc.GateMaxDurationMin);
  Serial.print(F("5 stdInitial\t\t"));
  Serial.println(voc.stdInitial);
  Serial.print(F("6 GainFactor\t\t"));
  Serial.println(voc.GainFactor);
  
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
  
  Serial.print(F("Provide the entry-number to change. Only <enter> is return. (or wait "));
  Serial.print(INPUTDELAY);
  Serial.println(" seconds)");
  while(1) {
    
    String Keyb = Serial.readStringUntil(0x0a);

    if (Keyb.length() < 2) return -100;
    int in = Keyb.toInt();
    
    if(in > maxop || in < minop) {
      Serial.print(in);
      Serial.print(F(" Invalid option. Min "));
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
