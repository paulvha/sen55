/*  
 *  version 1.0 / october 2024 / paulvha
 *    
 *  This example will connect to the SEN55. It will read the serialnumber, name and different software
 *  levels. 
 *  
 *  It will display the Mass, VOC, NOC, Temperature and humidity information, as well as display the BME280
 *  Humidity, Temperature, and pressure.
 *  
 *  Pressing t + <enter> during measurement will switch temperature reading between celcius and Fahrenheit.
 *  Pressing a + <enter> during measurment will switch between foot and meter for altitude.
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

#include "sen55.h"
#include "SparkFunBME280.h"

///////////////////////////////////////////////////////////////
//                          BME280                           //
///////////////////////////////////////////////////////////////
/* define the BME280 address.
 * Use if address jumper is closed (SDO - GND) : 0x76.*/
#define I2CADDR 0x77

/* Define reading in Fahrenheit or Celsius
 *  1 = Celsius
 *  0 = Fahrenheit */
#define TEMP_TYPE 1

/* define whether hight Meters or Foot
 *  1 = Meters
 *  0 = Foot */
#define BME_HIGHT 1

/////////////////////////////////////////////////////////////
/* define driver debug
 * 0 : no messages
 * 1 : request debug messages */
 //////////////////////////////////////////////////////////////
#define DEBUG 0

///////////////////////////////////////////////////////////////
/////////// NO CHANGES BEYOND THIS POINT NEEDED ///////////////
///////////////////////////////////////////////////////////////

SEN55 sen55;
BME280 mySensor; //Global sensor object
struct sen_values val;
bool detect_BME280 = false;
bool header = true;

int Type_temp = TEMP_TYPE;
int Type_hight = BME_HIGHT;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);

  serialTrigger((char *) "SEN55-Example3: Display basic values & BME280. press <enter> to start");

  Serial.println(F("Trying to connect."));

  // set library debug level
  sen55.EnableDebugging(DEBUG);

  Wire.begin();

  //********************** BME280 **************************************************

  // set BME280 I2C address.
  mySensor.setI2CAddress(I2CADDR);

  if (mySensor.beginI2C() == false) // Begin communication over I2C
    Serial.println("The BME280 did not respond. Please check wiring.");
  else
  {
    detect_BME280 = true;
    Serial.println(F("Detected BME280"));
  }

  //********************** SEN-55 **************************************************
  
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

  // reset SEN55
  if (! sen55.reset()) {
    Serial.println("could not reset SEN55.");
    while(1);
  }

  Display_Device_info();
}

void loop() {

  read_all();

  if (Serial.available()) {
    char c = Serial.read();
    flush;
    
    if (c == 't' || c== 'T') {
      if (Type_temp == 1) Type_temp = 0;
      else Type_temp = 1;
      header = true;
    }
    else if (c == 'a' || c== 'A') {
      if (Type_hight == 1) Type_hight = 0;
      else Type_hight = 1;
      header = true;
    }
    else {
      Serial.print("Ignoring ");
      Serial.println(c);
    }
  }
    
  delay(2000);
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
