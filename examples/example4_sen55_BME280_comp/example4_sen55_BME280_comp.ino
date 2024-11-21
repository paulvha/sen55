/*  
 *  version 1.0 / october 2024 / paulvha
 *    
 *  This example will connect to the SEN55. It will read the serialnumber, name and different software
 *  levels. 
 *  
 *  It will display the Mass, VOC, NOx, Temperature and humidity information, as well as display the BME280
 *  Humidity, Temperature, and pressure.
 *  
 *  During measurement you can press <enter> and a menu will appear to save/restore 3 different 
 *  TempCompensation algorithms to the SEN-55. 
 *  
 *  Also the following sketch setting can be done from the menu :
 *  Switch temperature between celcius and Fahrenheit display.
 *  Switch altitude between foot and meter display.
 *  
 *  All values will be reset to default are startup / reset.
 *  
 *  ============================================
 *  Temp compensation  (source datasheet SEN55)
 *  
 *  These commands allow to compensate temperature effects of the design-in at customer side 
 *  by applying a custom temperature offset to the ambient temperature. 
 *  The compensated ambient temperature is calculated as follows:
 *       T_Ambient_Compensated = T_Ambient + (slope*T_Ambient) + offset
 *       
 *  Where slope and offset are the values set with this command, smoothed with the specified 
 *  time constant. The time constant is how fast the slope and offset are applied. 
 *  After the speciﬁed value in seconds, 63% of the new slope and offset are applied.
 *  
 *  More details about the tuning of these parameters are included in the application note 
 *  “Temperature Acceleration and Compensation Instructions for SEN5x”. (see extra-folder)
 *  
 *  All temperatures (T_Ambient_Compensated,T_Ambient and offset) are represented in °C.
 *  
 *  Pressing enter during the measurement will display the current values and provide a menu to
 *  change the parameters that can be changed. 
 *  
 *  ===================================================
 *  Read and Update warm start (source datasheet SEN55)
 *  
 * The temperature compensation algorithm is optimized for a cold start by default, i.e., 
 * it is assumed that the "Start Measurement" commands are called on a device not yet 
 * warmed up by previous measurements. 
 * 
 * If the measurement is started on a device that is already warmed up, this parameter 
 * can be used to improve the initial accuracy of the ambient temperature output. 
 * 
 * This parameter can be gotten and set in any state of the device, 
 * but it is applied only the next time starting a measurement, 
 * i.e., when sending a "Start Measurement" command. 
 * So, the parameter needs to be written before a warm-start measurement is started.
 *  
 *  ====================================================
 * RHT acceleration algorithm (source datasheet SEN55)
 * 
 * By default, the RH/T acceleration algorithm is optimized for a sensor which is positioned in free air.
 * If the sensor is integrated into another device, the ambient RH/T output values might not be optimal 
 * due to different thermal behavior.
 * 
 * This parameter can be used to adapt the RH/T acceleration behavior for the actual use-case, leading in an
 * improvement of the ambient RH/T output accuracy. There is a limited set of different modes available, 
 * each identiﬁed by a number:
 *  • 0: Low Acceleration
 *  • 1: High Acceleration
 *  • 2: Medium Acceleration
 * 
 * Medium and high accelerations are particularly indicated for air quality monitors which are subjected 
 * to large temperature changes. Low acceleration is advised for stationary devices not subject to large 
 * variations in temperature.
 * 
 * This parameter can be changed in any state of the device, but it is applied only the next time starting 
 * a measurement, i.e., when sending a "Start Measurement" command. So, the parameter needs to be set 
 * before a new measurement is started.
 *  ====================================================
 *  
 * Tested on UNOR4, Artemis ATP, UNOR3, ATmega, Due, ESP32
 * 
 * ..........................................................
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
 *  ..........................................................
 *  Successfully tested on UNO R4 & Artemis/Apollo3 Sparkfun
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
 *   ===============  BME280 sensor =========================
 *  BME280
 *  VCC  ------ VCC  (5V)
 *  GND  ------ GND
 *  SCK  ------ SCL
 *  SDI  ------ SDA
 *  
 *  ADAFRUIT 
 *  BME280
 *  VIN ------ VCC  (5V)
 *  3V3        output (do not use)
 *  GND ------ GND
 *  SCK ------ SCL
 *  SDO        if connected to GND address is 0x76 instead of 0x77  
 *  SDI ------ SDA
 *  CS         do not connect
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

///////////////////////////////////////////////////////////////
//                          BME280                           //
///////////////////////////////////////////////////////////////
/* define the BME280 address.
 * Use if address jumper is closed (SDO - GND) : 0x76.*/
#define I2CADDR 0x77

/* Define default reading in Fahrenheit or Celsius
 *  1 = Celsius
 *  0 = Fahrenheit */
#define TEMP_TYPE 1

/* define default display alitude in Meters or Foot
 *  1 = Meters
 *  0 = Foot */
#define BME_HIGHT 1

/////////////////////////////////////////////////////////////
/* define driver debug
 * 0 : no messages
 * 1 : request debug messages */
 //////////////////////////////////////////////////////////////
#define DEBUG 0

/////////////////////////////////////////////////////////////
// wait max INPUTDELAY seconds on input
/////////////////////////////////////////////////////////////
#define INPUTDELAY 10

///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////

#include "sen55.h"
#include "SparkFunBME280.h"

//Global objects
SEN55 sen55;
BME280 mySensor; 

// global variables
struct sen_values val;
bool detect_BME280 = false;
bool header = true;

int Type_temp = TEMP_TYPE;
int Type_hight = BME_HIGHT;

struct sen_tmp_comp temp;
uint16_t WarmStart, AccMode;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);

  serialTrigger((char *) "SEN55-Example4: Display basic values & BME280 & compensation. press <enter> to start");

  Serial.println(F("Trying to connect."));

  // set library debug level
  sen55.EnableDebugging(DEBUG);

  Wire.begin();

  //********************** BME280 **************************************************

  // set BME280 I2C address.
  mySensor.setI2CAddress(I2CADDR);

  if (mySensor.beginI2C() == false) // Begin communication over I2C
    Serial.println(F("The BME280 did not respond. Please check wiring."));
  else
  {
    detect_BME280 = true;
    Serial.println(F("Detected BME280."));
  }

  //********************** SEN-55 **************************************************
  
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

  read_all();

  // trigger reading adjusting Temperature compensation (pressed <enter>)
  if (Serial.available()) {
    flush();
    CheckTmpValues();
  }
  else
    delay(2000);
}

void CheckTmpValues() {
  int inp;
  
  if(sen55.GetTmpComp(&temp) != SEN55_ERR_OK){
    Serial.println(F("Could not read temperature compensation."));
    return; 
  }

  if(sen55.GetWarmStart(&WarmStart) != SEN55_ERR_OK){
    Serial.println(F("Could not warmstart compensation."));
    return; 
  }

  if(sen55.GetRHTAccelMode(&AccMode) != SEN55_ERR_OK){
    Serial.println(F("Could not Acceleration mode."));
    return; 
  }

  while(1) {
    
    disp_tmp_info();

    inp = GetInput(1,9);

    // cancel (or timeout)
    if (inp == 7 || inp == -100) {
      Serial.println(F("Cancel"));
      return;
    }
    
    Serial.println(inp);
  
    // update values
    if (inp == 6) {
    
      // write back
      if (sen55.SetTmpComp(&temp) != SEN55_ERR_OK)
        Serial.println(F("Could not update temperature compensation."));
      else
        Serial.println(F("Updated temperature compensation."));

      if (sen55.SetWarmStart(WarmStart) != SEN55_ERR_OK)
        Serial.println(F("Could not update warmstart compensation."));
      else
        Serial.println(F("Updated warmstart compensation."));

      if (sen55.SetRHTAccelMode(AccMode) != SEN55_ERR_OK)
        Serial.println(F("Could not update Acceleration mode."));
      else
        Serial.println(F("Updated Acceleration mode."));

      header = true;
      
      return;
    }

    else if (inp == 1) {
      Serial.print(F("1 Temperature offset\t"));
      Serial.print(temp.offset);
      Serial.println("  range -10..10"); // articifial limit
      inp = GetInput(-10,10);
      if (inp == -100) continue;
      temp.offset = inp;
    }

    else if (inp == 2) {
      Serial.print(F("2 Temperature slope\t"));
      Serial.print(temp.slope);
      Serial.println(" range 0..50"); // articifial limit
      inp = GetInput(0,50);
      if (inp == -100) continue;
      temp.slope = inp;
    }

    else if (inp == 3) {
      Serial.print(F("3 Temperature time (seconds)\t"));
      Serial.print(temp.time);
      Serial.println(" range 0..10"); // articifial limit
      inp = GetInput(0,10);
      if (inp == -100) continue;
      temp.time = inp;
    }
    
    else if (inp == 4) {
      Serial.print(F("4 WarmStart compensation\t"));
      Serial.print(WarmStart);
      Serial.println(" range 0..65535");
      inp = GetInput(0,65535);
      if (inp == -100) continue;
      
      if (WarmStart != inp) { 
        // only impact at new start measurement
        // (re)start will happen with the next measurement
        if (! sen55.stop()) {
          Serial.println(F("Could not Stop measurement."));
        }
      }
      
      WarmStart = inp;
    }
    
    else if (inp == 5) {
      Serial.print(F("5 Acceleration mode\t"));
      Serial.print(AccMode);
      Serial.println(" range 0..2");
      inp = GetInput(0,2);
      if (inp == -100) continue;
      
      if (AccMode != inp) { 
        // only impact at new start measurement
        // (re)start will happen with the next measurement
        if (! sen55.stop()) {
          Serial.println(F("Could not Stop measurement."));
        }
      }
      
      AccMode = inp;
    }
    
    else if (inp == 8) {
      if (Type_temp == 1) Type_temp = 0;
      else Type_temp = 1;
      header = true;
    }

    else if (inp == 9) {
      if (Type_hight == 1) Type_hight = 0;
      else Type_hight = 1;
      header = true;
    }
  }
}

void disp_tmp_info()
{
  Serial.println(F("\n************************\n"));
  Serial.print(F("1 Temperature offset\t "));
  Serial.println(temp.offset);
  Serial.print(F("2 Temperature slope\t "));
  Serial.println(temp.slope);
  Serial.print(F("3 Temperature time\t "));
  Serial.println(temp.time);
  Serial.print(F("4 WarmStart compensation "));
  Serial.println(WarmStart);
  Serial.print(F("5 Acceleration mode\t "));
  Serial.println(AccMode);
  Serial.println(F("6 Update Sen55 values(MUST do to enable above changes)\n"));
  Serial.println(F("7 Cancel & disgard above changes"));

  Serial.println(F("\nFollowing changes are applied immediately:"));
  if (Type_temp == 1)  Serial.println(F("8 Switch to display Fahrenheit"));
  else Serial.println(F("8 Switch to display Celcius"));
  if (Type_hight == 1)  Serial.println(F("9 Switch to display in foot\n"));
  else Serial.println(F("9 Switch to display in meters\n"));
}

/**
 * @brief : read and display all values
 */
bool read_all()
{
  uint8_t ret, error_cnt = 0;

  // loop to get data
  do {

    ret = sen55.GetValues(&val);

    // data might not have been ready
    if (ret == SEN55_ERR_DATALENGTH){

      if (error_cnt++ > 3) {
        Serial.print(F("Error during reading values: "));
        Serial.println(ret);
        return(false);
      }
      delay(1000);
    }

    // if other error
    else if(ret != SEN55_ERR_OK) {
      Serial.print(F("Error during reading values: "));
      Serial.println(ret);
      Check_error();
      return(false);
    }

  } while (ret != SEN55_ERR_OK);

  // only print header first time
  if (header) {

    Serial.print(F("==================================== SEN-55 ========================   "));
    if(detect_BME280) Serial.print(F("================= BME280 =============="));

    Serial.print(F("\n-------------Mass -----------    VOC:  NOX:  Humidity:  Temperature:"));
    if(detect_BME280) Serial.print(F("   Humidity  Temperature Pressure Altitude"));

    Serial.print(F("\n     Concentration [μg/m3]      index  index   %         "));
    
    if (Type_temp) Serial.print(F("[*C]"));
    else Serial.print(F("[*F]"));
      
    if(detect_BME280) {
      
      if (Type_temp) Serial.print(F("\t\t[%]      [*C]"));
      else Serial.print(F("\t\t[%]      [*F]"));

      Serial.print(F("     [hPa]\t\t"));

      if (Type_hight) Serial.print(F("Meter\t"));
      else Serial.print(F("Foot\t"));
    }

    Serial.println(F("\nP1.0\tP2.5\tP4.0\tP10\t\n"));

    header = false;
  }

  Serial.print(val.MassPM1);
  Serial.print(F("\t"));
  Serial.print(val.MassPM2);
  Serial.print(F("\t"));
  Serial.print(val.MassPM4);
  Serial.print(F("\t"));
  Serial.print(val.MassPM10);
  Serial.print(F("\t")); 
  Serial.print(val.VOC);
  Serial.print(F("\t"));
  Serial.print(val.NOX);
  Serial.print(F("\t"));
  Serial.print(val.Hum);
  Serial.print(F("\t"));
  if (Type_temp) Serial.print(val.Temp,2);
  else Serial.print( ((val.Temp *9)/5 + 32 ),2);

  if(detect_BME280) {
    Serial.print(F("\t\t"));
    Serial.print(mySensor.readFloatHumidity(), 1);
    Serial.print(F("\t  "));
    
    if (Type_temp) Serial.print(mySensor.readTempC(), 2);
    else Serial.print(mySensor.readTempF(), 2);
    Serial.print(F("\t    "));
    
    Serial.print(mySensor.readFloatPressure()/100, 0);
    Serial.print(F("\t"));

    if (Type_hight) Serial.print(mySensor.readFloatAltitudeMeters(), 1);
    else Serial.print(mySensor.readFloatAltitudeFeet(), 1);
  }

  Serial.print(F("\n"));

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
  
  Serial.print(F("Provide the number. Only <enter> is return. (or wait "));
  Serial.print(INPUTDELAY);
  Serial.println(F(" seconds)"));
  
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
