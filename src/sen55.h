/**
 * SEN55 Library Header file
 *
 * Copyright (c) October 2024, Paul van Haastrecht
 *
 * All rights reserved.
 *
 * Development environment specifics:
 * Arduino IDE 1.8.16 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 * Version 1.0 / October 2024 
 * - Initial version by paulvha
 *********************************************************************
*/
#ifndef SEN55_H
#define SEN55_H

#include <Arduino.h>                // Needed for Stream

/**
 * library version levels
 */
#define DRIVER_MAJOR 1
#define DRIVER_MINOR 0

/**
 * select default debug serial
 */
#define SEN55_DEBUGSERIAL Serial

/**
 * If the platform is an ESP32 AND it is planned to connect an SCD30 as well,
 * you have to remove the comments from the line below
 *
 * The standard I2C on an ESP32 does NOT support clock stretching
 * which is needed for the SCD30. You must have SCD30 library downloaded
 * from https://github.com/paulvha/scd30 and included in your sketch
 * (see examples)
 */
//#define SCD30_SEN55_ESP32 1

#if defined SCD30_SEN55_ESP32   // in case of use in combination with SCD30
  #include <SoftWire/SoftWire.h>
#else
  #include "Wire.h"            // for I2c
#endif

/**
 * Auto detect that some boards have low memory. (like Uno)
 */

#if defined (__AVR_ATmega328__) || defined(__AVR_ATmega328P__)|| defined(__AVR_ATmega16U4__) || (__AVR_ATmega32U4__)
  #define SMALLFOOTPRINT 1
#endif 

/**
 * An AVR has 32 I2C buffer. For reading values that is enough, but the Serial number and name, both can have 32 characters. As
 * after each 2 characters a CRC-byte is added, the total to read becomes 48. THus reading will fail.
 * 
 * This check will enable to expect (and read) max 32 bytes in that case.
 */
 
#if defined ARDUINO_ARCH_AVR
  #define MAX_32_TO_EXPECT 1
#endif

/* structure to return mass values */
struct sen_values {
  float   MassPM1;        // Mass Concentration PM1.0 [μg/m3]
  float   MassPM2;        // Mass Concentration PM2.5 [μg/m3]
  float   MassPM4;        // Mass Concentration PM4.0 [μg/m3]
  float   MassPM10;       // Mass Concentration PM10 [μg/m3]
  float   Hum;            // Compensated Ambient Humidity [%RH]
  float   Temp;           // Compensated Ambient Temperature [°C]
  float   VOC;            // VOC Index
  float   NOX;            // NOx Index
};

/* structure to return PM values */
struct sen_values_pm {
  float   MassPM1;        // Mass Concentration PM1.0 [μg/m3]
  float   MassPM2;        // Mass Concentration PM2.5 [μg/m3]
  float   MassPM4;        // Mass Concentration PM4.0 [μg/m3]
  float   MassPM10;       // Mass Concentration PM10 [μg/m3]
  float   NumPM0;         // Number Concentration PM0.5 [#/cm3]
  float   NumPM1;         // Number Concentration PM1.0 [#/cm3]
  float   NumPM2;         // Number Concentration PM2.5 [#/cm3]
  float   NumPM4;         // Number Concentration PM4.0 [#/cm3]
  float   NumPM10;        // Number Concentration PM4.0 [#/cm3]
  float   PartSize;       // Typical Particle Size [μm]
};

/**
 * Obtain different version levels
 */
struct sen_version {
  uint8_t F_major;        // Firmware level SEN55
  uint8_t F_minor;
  bool F_debug;           // Firmware in debug state   (NOT DOCUMENTED)
  uint8_t H_major;        // Hardware level SEN55      (NOT DOCUMENTED)
  uint8_t H_minor;
  uint8_t P_major;        // protocol level SEN55      (NOT DOCUMENTED)
  uint8_t P_minor;
  uint8_t L_major;        // library level
  uint8_t L_minor;
};

/**
 * Used to read / write the Nox values
 * More details on the tuning instructions are provided in 
 * the application note “Engineering Guidelines for SEN5x"
 * (see extra-folder in this library)
 */
struct sen_xox {
  /** NOx index representing typical (average) conditions. Allowed
   * values are in range 1..250. The default value is 1. */
  int16_t IndexOffset;
  
  /** Time constant to estimate the NOx algorithm offset from the
   * history in hours. Past events will be forgotten after about twice the
   * learning time. Allowed values are in range 1..1000. 
   * The default value is 12 hours */
  int16_t LearnTimeOffsetHours;
  
  /** The time constant to estimate the NOx algorithm gain from the
   * history has no impact for NOx. This parameter is still in place for
   * consistency reasons with the VOC tuning parameters command.
   * =========================================================
   * !! This parameter must always be set to 12 hours.!!!! 
   * =========================================================*/
  int16_t LearnTimeGainHours;
  
  /** Maximum duration of gating in minutes (freeze of estimator during
   * high NOx index signal). Set to zero to disable the gating. Allowed
   * values are in range 0..3000. The default value is 720 minutes.*/
  int16_t GateMaxDurationMin;
  
  /** The initial estimate for standard deviation parameter has no
   * impact for NOx. This parameter is still in place for consistency
   * reasons with the VOC tuning parameters command. 
   * =========================================================
   * !!! This parameter must always be set to 50
   * =========================================================*/
  int16_t stdInitial;
  
  /** Gain factor to amplify or to attenuate the NOx index output.
   * Allowed values are in range 1..1000. The default value is 230.*/
  int16_t GainFactor;
};

/**
 * Temperature compensation
 * More details about the tuning of these parameters are included 
 * in the application note “Temperature Acceleration and
 * Compensation Instructions for SEN5x”.
 */ 
struct sen_tmp_comp {
  /**Temperature offset [°C] (default value: 0)
   * scale 200*/
  int16_t offset;
  
  /**Normalized temperature offset slope (default value: 0)
   * scale 1000*/
  int16_t slope;
  
  /** Time constant in seconds (default value: 0) 
   * scale 1
   * Where slope and offset are the values set with this command, 
   * smoothed with the specified time constant. The time constant 
   * is how fast the slope and offset are applied. 
   * After the speciﬁed value in seconds, 63% of the new slope 
   * and offset are applied.*/
  uint16_t time;
};

#ifndef SMALLFOOTPRINT

  // error description
  struct SEN55_Description {
    uint8_t code;
    char    desc[80];
  };
#endif

/*************************************************************/

/**
 * Status register Error result
 *
 * REQUIRES FIRMWARE LEVEL 2.x
 */
enum SEN_status {
  STATUS_OK_55 = 0,
  STATUS_SPEED_ERROR_55 =     0b00000001,
  STATUS_LASER_ERROR_55 =     0b00000010,
  STATUS_FAN_ERROR_55 =       0b00000100,
  STATUS_GAS_ERROR_55 =       0b00001000, 
  STATUS_RHT_ERROR_55 =       0b00010000,
  STATUS_FAN_CLEAN_ACTIVE_55 =0b00100000
};

/**
 * commands for the SEN55
 */
#define SEN55_START_MEASUREMENT       0x0021
#define SEN55_START_RHTG_MEASUREMENT  0x0037    // WILL NOT START LASER
#define SEN55_STOP_MEASUREMENT        0x0104
#define SEN55_READ_DATA_RDY_FLAG      0x0202
#define SEN55_READ_MEASURED_VALUE     0x03C4
#define SEN55_READ_MEASURED_VALUE_PM  0x0413    // NOT DOCUMENTED
#define SEN55_TEMP_COMP               0X60B2
#define SEN55_WARM_START_PARAM        0X60C6
#define SEN55_VOC_TUNING              0X60D0
#define SEN55_NOX_TUNING              0X60E1
#define SEN55_RHT_ACCEL               0X60F7
#define SEN55_VOC_ALGO                0X6181
#define SEN55_START_FAN_CLEANING      0x5607
#define SEN55_AUTO_CLEANING_INTERVAL  0x8004
#define SEN55_READ_PRODUCT_NAME       0xD014  
#define SEN55_READ_SERIAL_NUMBER      0xD033
#define SEN55_READ_VERSION            0xD100
#define SEN55_READ_DEVICE_REGISTER    0xD206
#define SEN55_CLEAR_DEVICE_REGISTER   0xD210
#define SEN55_RESET                   0xD304

// Write helpers
#define SEN55_SET_AUTO_CLEANING_INTERVAL  0x55FF
#define SEN55_SET_VOC_ALGO            0x55FE
#define SEN55_SET_NOX_TUNING          0x55FD
#define SEN55_SET_TEMP_COMP           0x55FC
#define SEN55_SET_WARM_START_PARAM    0x55FB
#define SEN55_SET_RHT_ACCEL           0x55FA
#define SEN55_SET_VOC_TUNING          0x55F9

/**
 * error codes 
 */
#define SEN55_ERR_OK          0x00
#define SEN55_ERR_DATALENGTH  0X01
#define SEN55_ERR_UNKNOWNCMD  0x02
#define SEN55_ERR_ACCESSRIGHT 0x03
#define SEN55_ERR_PARAMETER   0x04
#define SEN55_ERR_OUTOFRANGE  0x28
#define SEN55_ERR_CMDSTATE    0x43
#define SEN55_ERR_TIMEOUT     0x50
#define SEN55_ERR_PROTOCOL    0x51
#define SEN55_ERR_FIRMWARE    0x88

// Receive buffer length.
// in case of name / serial number the max is 32 + 16 CRC = 48
#define MAXBUFLENGTH 50

// I2c fixed address
#define SEN55_ADDRESS 0x69            

class SEN55
{
  public:

    SEN55(void);
    
    /**
    * @brief  Enable or disable the printing of sent/response HEX values.
    *
    * @param act : level of debug to set
    *  0 : no debug message
    *  1 : sending and receiving data
    */
    void EnableDebugging(uint8_t act);

    /**
     * @brief Manual assigment I2C communication port 
     *
     * @param port : I2C communication channel to be used
     *
     * User must have preformed the wirePort.begin() in the sketch.
     */
     
    bool begin(TwoWire *wirePort);

    /**
     * @brief : Perform SEN55 instructions
     */
    bool probe();
    bool reset() {return(Instruct(SEN55_RESET));}
    bool start() {return(Instruct(SEN55_START_MEASUREMENT));}
    bool startRHTG() {return(Instruct(SEN55_START_RHTG_MEASUREMENT));}
    bool stop()  {return(Instruct(SEN55_STOP_MEASUREMENT));}
    bool clean() {return(Instruct(SEN55_START_FAN_CLEANING));}

    /**
     * @brief : Set or get Auto Clean interval
     */
    uint8_t GetAutoCleanInt(uint32_t *val);
    uint8_t SetAutoCleanInt(uint32_t val);

    /**
     * @brief : retrieve Error message details
     */
    void GetErrDescription(uint8_t code, char *buf, int len);

    /**
     * @brief : retrieve device information from the sen55
     * 
     * @param ser     : buffer to hold the read result
     * @param len     : length of the buffer (max 32 char)
     */
    uint8_t GetSerialNumber(char *ser, uint8_t len) {return(Get_Device_info(SEN55_READ_SERIAL_NUMBER, ser, len));}
    uint8_t GetProductName(char *ser, uint8_t len)  {return(Get_Device_info(SEN55_READ_PRODUCT_NAME, ser, len));} 

    /**
     * @brief : retrieve version information from the SEN55 and library
     * 
     * @return
     *  SEN55_ERR_OK = ok
     *  else error
     */
    uint8_t GetVersion(struct sen_version *v);

    /** 
     * @brief : Read Device Status from the SEN55
     *
     * @param  *status
     *  return status as an 'or':
     *   STATUS_OK_55 = 0,
     *   STATUS_SPEED_ERROR_55 =     0b00000001,
     *   STATUS_LASER_ERROR_55 =     0b00000010,
     *   STATUS_FAN_ERROR_55 =       0b00000100,
     *   STATUS_GAS_ERROR_55 =       0b00001000, 
     *   STATUS_RHT_ERROR_55 =       0b00010000,
     * 
     * Only if return is STATUS_OK_55 the fan status COULD be set
     *   STATUS_FAN_CLEAN_ACTIVE_55 =0b00100000
     *
     * @return
     *  STATUS_OK_55 = ok, no isues found
     *  else SEN55_ERR_OUTOFRANGE, issues found
     */
    uint8_t GetStatusReg(uint8_t *status);

    /**
     * @brief : retrieve measurement values from SEN55
     * 
     * @param laser : 
     * true : mass and RHTG
     * false : RHTG only (NO laser start)
     * 
     * @return
     *  STATUS_OK_55 = ok
     *  else error
     */
    uint8_t GetValues(struct sen_values *v, bool laser = true);

    /**
     * @brief : retrieve all measurement values from SEN55
     * laser values only: mass, num and partsize (compatible with SPS30)
     * @param 
     *  v: pointer to structure to store
     *
     * return
     *  SEN55_ERR_OK = ok
     *  else error
     */
    uint8_t GetValuesPM(struct sen_values_pm *v);
  
    /**
     * @brief : save or restore the VOC algorithm 
     * 
     * @param :
     * table : to hold the values
     * tablesize : size of table ( must be at least VOC_ALO_SIZE)
     * 
     * @return :
     * SEN55_ERR_OK : all OK
     * else error
     */ 
    #define VOC_ALO_SIZE 8    // is 8 NOT 10 as in the datasheet !!
    uint8_t SetVocAlgorithmState(uint8_t *table, uint8_t tablesize);
    uint8_t GetVocAlgorithmState(uint8_t *table, uint8_t tablesize);

    /**
     * @brief : save or restore the Nox algorithm 
     * 
     * @param :
     * Nox : structure to hold the values
     * 
     * @return :
     * SEN55_ERR_OK : all OK
     * else error
     */ 
    uint8_t GetNoxAlgorithm(sen_xox *nox);
    uint8_t SetNoxAlgorithm(sen_xox *nox);
    
    /**
     * @brief : save or restore the Nox algorithm 
     * 
     * @param :
     * voc : structure to hold the values
     * 
     * @return :
     * SEN55_ERR_OK : all OK
     * else error
     */ 
    uint8_t GetVocAlgorithm(sen_xox *voc);
    uint8_t SetVocAlgorithm(sen_xox *voc);


    /**
     * @brief : Read and Update the Temperature compensation 
     * 
     * @param :
     * tmp : structure to hold the values
     * 
     * @return :
     * SEN55_ERR_OK : all OK
     * else error
     */ 
    uint8_t GetTmpComp(sen_tmp_comp *tmp);
    uint8_t SetTmpComp(sen_tmp_comp *tmp);

    /**
     * @brief : Read and Update warm start 
     * The temperature compensation algorithm is optimized for a cold 
     * start by default, i.e., it is assumed that the "Start Measurement" 
     * commands are called on a device not yet warmed up by previous measurements. 
     * 
     * If the measurement is started on a device that is already warmed up, 
     * this parameter can be used to improve the initial accuracy of the ambient 
     * temperature output. 
     * 
     * This parameter can be gotten and set in any state of the device, 
     * but it is applied only the next time starting a measurement, 
     * i.e., when sending a "Start Measurement" command. 
     * So, the parameter needs to be written before a warm-start measurement is started.
     * 
     * @param :
     * val : to read or write
     * Warm start behavior as a value in the range from 0 (cold start, default 
     * value) to 65535 (warm start). (default value: 0)
     * 
     * @return :
     * SEN55_ERR_OK : all OK
     * else error
     */    
    uint8_t GetWarmStart(uint16_t *val);
    uint8_t SetWarmStart(uint16_t val);

    /**
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
     */ 
    uint8_t GetRHTAccelMode(uint16_t *val);
    uint8_t SetRHTAccelMode(uint16_t val); 
  
  private:
    /** debug */
    void DebugPrintf(const char *pcFmt, ...);
    int _SEN55_Debug;                   // program debug level
    char prfbuf[256];
    
    /** shared variables */
    uint8_t _Receive_BUF[MAXBUFLENGTH]; // buffers
    uint8_t _Send_BUF[MAXBUFLENGTH];
    uint8_t _Receive_BUF_Length;
    uint8_t _Send_BUF_Length;
    bool _started;                      // indicate the measurement has started
    uint8_t _FW_Major, _FW_Minor;       // holds sen55 firmware level
    uint32_t data32;                    // pass data to i2c_fill_buffer
    uint16_t data16;
    
    /* needed for auto interval timing */
    typedef union {
      byte array[4];
      uint32_t value;
    } ByteToU32_55;
    
    /** shared supporting routines */
    uint8_t Get_Device_info(uint16_t type, char *ser, uint8_t len);
    bool Instruct(uint16_t type);
    bool FWCheck(uint8_t major, uint8_t minor); 
    uint32_t byte_to_U32(int x);
    uint16_t byte_to_Uint16_t(int x);
    int16_t byte_to_int16_t(int x);
    bool Check_data_ready();
    
    /** I2C communication */
    TwoWire *_i2cPort;                  // holds the I2C port
    void I2C_init();
    void I2C_fill_buffer(uint16_t cmd, void *val = NULL);
    uint8_t I2C_ReadToBuffer(uint8_t count, bool chk_zero);
    uint8_t I2C_SetPointer_Read(uint8_t cnt, bool chk_zero = false);
    uint8_t I2C_SetPointer();
    uint8_t I2C_calc_CRC(uint8_t data[2]);
};
#endif /* SEN55_H */
