/**
 * SEN55 Library file
 *
 * Copyright (c) October 2024, Paul van Haastrecht
 *
 * All rights reserved.
 *
 * The library can communicated with the SEN55 to get and set information. 
 *
 * Development environment specifics:
 * Arduino IDE 1.8.16
 *
 * ================ Disclaimer ===================================
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 **********************************************************************
 * Version 1.0 / October 2024 /paulvha
 * - Initial version
 *
 *********************************************************************
 */

#include "sen55.h"
#include <stdarg.h>
#include <stdio.h>

#if not defined SMALLFOOTPRINT
/* error descripton */
struct SEN55_Description SEN55_ERR_desc[11] =
{
  {SEN55_ERR_OK, "All good"},
  {SEN55_ERR_DATALENGTH, "Wrong data length for this command (too much or little data)"},
  {SEN55_ERR_UNKNOWNCMD, "Unknown command"},
  {SEN55_ERR_ACCESSRIGHT, "No access right for command"},
  {SEN55_ERR_PARAMETER, "Illegal command parameter or parameter out of allowed range"},
  {SEN55_ERR_OUTOFRANGE, "Internal function argument out of range"},
  {SEN55_ERR_CMDSTATE, "Command not allowed in current state"},
  {SEN55_ERR_TIMEOUT, "No response received within timeout period"},
  {SEN55_ERR_PROTOCOL, "Protocol error"},
  {SEN55_ERR_FIRMWARE, "Not supported on this SEN55 firmware level"},
  {0xff, "Unknown Error"}
};
#endif // SMALLFOOTPRINT

/**
 * @brief constructor and initialize variables
 */
SEN55::SEN55(void)
{
  _Send_BUF_Length = 0;
  _Receive_BUF_Length = 0;
  _SEN55_Debug = 0;
  _started = false;
  _FW_Major = _FW_Minor = 0;
}

/**
 * @brief  Enable or disable the printing of sent/response HEX values.
 *
 * @param act : level of debug to set
 *  0 : no debug message
 *  1 : sending and receiving data
 */
void SEN55::EnableDebugging(uint8_t act) {
  _SEN55_Debug = act;
}

/**
 * @brief Print debug message if enabled 
 */
void SEN55::DebugPrintf(const char *pcFmt, ...)
{
  va_list pArgs;
  
  if (_SEN55_Debug == 0) return;
  
  va_start(pArgs, pcFmt);
  vsprintf(prfbuf, pcFmt, pArgs);
  va_end(pArgs);

  SEN55_DEBUGSERIAL.print(prfbuf);
}

/**
 * @brief begin communication
 *
 * @param port : I2C communication channel to be used
 *
 * User must have preform the wirePort.begin() in the sketch.
 */
bool SEN55::begin(TwoWire *wirePort)
{
  _i2cPort = wirePort;            // Grab which port the user wants us to use
  _i2cPort->setClock(100000);     // some boards do not set 100K
  return true;
}

/**
 * @brief check if SEN55 sensor is available (read version number)
 *
 * Return:
 *   true on success else false
 */
bool SEN55::probe() {
  
  struct sen_version v;

  if (GetVersion(&v) == SEN55_ERR_OK)  return(true);

  return(false);
}

/**
 * @brief Check Firmware level
 *
 * @param  Major : minimum Major level of firmware
 * @param  Minor : minimum Minor level of firmware
*
 * return
 *  True if SEN55 has required firmware
 *  False does not have required firmware level.
 *
 */
bool SEN55::FWCheck(uint8_t major, uint8_t minor) {

  // do we have the current FW level
  if (_FW_Major == 0) {
      if (! probe()) return (false);
  }

  // if requested level is HIGHER than current
  if (major > _FW_Major || minor > _FW_Minor) return(false);

  return(true);
}

/**
 * @brief Read status register
 *
 * @param  *status
 *  return status as an 'or':
    STATUS_OK_55 = 0,
    STATUS_SPEED_ERROR_55 =     0b00000001,
    STATUS_LASER_ERROR_55 =     0b00000010,
    STATUS_FAN_ERROR_55 =       0b00000100,
    STATUS_GAS_ERROR_55 =       0b00001000, 
    STATUS_RHT_ERROR_55 =       0b00010000,
    STATUS_FAN_CLEAN_ACTIVE_55 =0b00100000
 *
 * Return obtain result
 * return
 *  SEN55_ERR_OK = ok, no isues found (status will updated for clean active)
 *  else SEN55_ERR_OUTOFRANGE, issues found
 */
uint8_t SEN55::GetStatusReg(uint8_t *status) {
  uint8_t ret;

  *status = STATUS_OK_55;

  // check for minimum Firmware level
  if(! FWCheck(2,0)) return(SEN55_ERR_FIRMWARE);

  // try to read status register
  I2C_fill_buffer(SEN55_READ_DEVICE_REGISTER);
  ret = I2C_SetPointer_Read(4,false);
  
  // clear status register just in case there was an issue
  I2C_fill_buffer(SEN55_CLEAR_DEVICE_REGISTER);
  I2C_SetPointer();

  if (ret != SEN55_ERR_OK) return (ret);

  if (_Receive_BUF[1] & 0b00100000) *status |= STATUS_SPEED_ERROR_55;
  if (_Receive_BUF[3] & 0b10000000) *status |= STATUS_GAS_ERROR_55;
  if (_Receive_BUF[3] & 0b01000000) *status |= STATUS_RHT_ERROR_55;
  if (_Receive_BUF[3] & 0b00100000) *status |= STATUS_LASER_ERROR_55;
  if (_Receive_BUF[3] & 0b00010000) *status |= STATUS_FAN_ERROR_55;
  
  if (*status != STATUS_OK_55) return(SEN55_ERR_OUTOFRANGE);
  
  // NO errors, now add / check that fan clean is active
  if (_Receive_BUF[1] & 0b00001000) *status = STATUS_FAN_CLEAN_ACTIVE_55;

  return(SEN55_ERR_OK);
}

/**
 * @brief Instruct SEN55 sensor
 * @param type
 *  SEN55_START_MEASUREMENT   Start measurement
 *  SEN55_START_RHTG_MEASUREMENT ( No laser)
 *  SEN55_STOP_MEASUREMENT    Stop measurement
 *  SEN55_RESET               Perform reset
 *  SEN55_START_FAN_CLEANING  start cleaning
 *
 * Return
 *  true = ok
 *  false = error
 */
bool SEN55::Instruct(uint16_t type)
{
  uint8_t ret;

  if (type == SEN55_START_FAN_CLEANING)
  {
    if ( ! _started )
    {
      DebugPrintf("ERROR: Sensor is not in measurement mode\n");
      return(false);
    }
  }

  I2C_fill_buffer(type);

  ret = I2C_SetPointer();

  if (ret == SEN55_ERR_OK) {

    if (type == SEN55_START_MEASUREMENT || type == SEN55_START_RHTG_MEASUREMENT) {
      _started = true;
      delay(1000);            // needs at least 20ms, we give plenty of time
    }
    else if (type == SEN55_STOP_MEASUREMENT)
      _started = false;

    else if (type == SEN55_RESET){
      _started = false;
      
      delay(500); //support for UNOR4 (else it will fail)
      _i2cPort->begin();       // some I2C channels need a reset
      delay(500); //support for UNOR4
    }

    return(true);
  }

  DebugPrintf("Instruction failed\n");
  return(false);
}

/**
 * @brief Read version info
 *
 * @return
 *  SEN55_ERR_OK = ok
 *  else error
 */
uint8_t SEN55::GetVersion(struct sen_version *v) {
 
  uint8_t ret; 

  memset(v, 0x0, sizeof(struct sen_version));

  I2C_fill_buffer(SEN55_READ_VERSION);

  ret = I2C_SetPointer_Read(8,false);

  if( ret  == SEN55_ERR_OK) {
    v->F_major = _Receive_BUF[0];
    v->F_minor = _Receive_BUF[1];
    v->F_debug = _Receive_BUF[2];
    v->H_major = _Receive_BUF[3];
    v->H_minor = _Receive_BUF[4];
    v->P_major = _Receive_BUF[5];
    v->P_minor = _Receive_BUF[6];
    v->L_major = DRIVER_MAJOR;
    v->L_minor = DRIVER_MINOR;
  
    // internal libary use
    _FW_Major = v->F_major;
    _FW_Minor = v->F_minor;
  }
  return(ret);
}

/**
 * @brief General Read device info
 *
 * @param type:
 *  Product Name  : SER_READ_DEVICE_PRODUCT_TYPE
 *  Serial Number : SER_READ_DEVICE_SERIAL_NUMBER
 *
 * @param ser     : buffer to hold the read result
 * @param len     : length of the buffer
 *
 * return
 *  SEN55_ERR_OK = ok
 *  else error
 */
uint8_t SEN55::Get_Device_info(uint16_t type, char *ser, uint8_t len)
{
  uint8_t ret,i;

  // Serial or name code
  if (type == SEN55_READ_SERIAL_NUMBER || type == SEN55_READ_PRODUCT_NAME) {
    I2C_fill_buffer(type);

    // true = check zero termination
    ret =  I2C_SetPointer_Read(len,true);
  }
  else
    ret = SEN55_ERR_PARAMETER;

  if (ret == SEN55_ERR_OK) {
    // get data
    for (i = 0; i < len ; i++) {
      ser[i] = _Receive_BUF[i];
      if (ser[i] == 0x0) break;
    }
  }

  return(ret);
}

/**
 * @brief : SET the auto clean interval
 * @param val : The new interval value
 * 6.1.13 Read/Write Auto Cleaning Interval
 * Return:
 *  OK = SEN55_ERR_OK
 *  else error
 */
uint8_t SEN55::SetAutoCleanInt(uint32_t val)
{
  bool save_started = false; 
  bool r = true;

  // change can only be applied when idle
  if (_started) {
    if (! stop()) return(SEN55_ERR_CMDSTATE);
    save_started = true;
  }

  I2C_fill_buffer(SEN55_SET_AUTO_CLEANING_INTERVAL);

  if (I2C_SetPointer() == SEN55_ERR_OK)
  {
    if (save_started) r = start();

    if (r) return(SEN55_ERR_OK);
  }

  return(SEN55_ERR_PROTOCOL);
}

uint8_t SEN55::GetWarmStart(uint16_t * val)
{
  uint8_t ret;

  I2C_fill_buffer(SEN55_WARM_START_PARAM);

  ret = I2C_SetPointer_Read(2);

  // get data
  *val = byte_to_Uint16_t(0);

  return(ret);
}

uint8_t SEN55::SetWarmStart(uint16_t val) {
  
  data16 = val;

  I2C_fill_buffer(SEN55_SET_WARM_START_PARAM);

  return(I2C_SetPointer());
}

uint8_t SEN55::GetRHTAccelMode(uint16_t *val){
  uint8_t ret;

  I2C_fill_buffer(SEN55_RHT_ACCEL);

  ret = I2C_SetPointer_Read(2);

  // get data
  *val = byte_to_Uint16_t(0);

  return(ret);
}
  
uint8_t SEN55::SetRHTAccelMode(uint16_t val) {
  data16 = val;

  I2C_fill_buffer(SEN55_SET_RHT_ACCEL);

  return(I2C_SetPointer());
}

/**
 * @brief : read the auto clean interval
 * @param val : pointer to return the interval value
 *
 * The default cleaning interval is set to 604’800 seconds (i.e., 168 hours or 1 week).
 * 6.1.13 Read/Write Auto Cleaning Interval
 * Return:
 *  OK = SEN55_ERR_OK
 *  else error
 */
uint8_t SEN55::GetAutoCleanInt(uint32_t *val)
{
  uint8_t ret;

  I2C_fill_buffer(SEN55_AUTO_CLEANING_INTERVAL);

  ret = I2C_SetPointer_Read(4);

  // get data
  *val = byte_to_U32(0);

  return(ret);
}

uint8_t SEN55::GetVocAlgorithmState(uint8_t *table, uint8_t tablesize) {
  uint8_t ret;
  
  // Check for Voc Algorithm length
  if (tablesize < VOC_ALO_SIZE) return(SEN55_ERR_PARAMETER);
  
  I2C_fill_buffer(SEN55_VOC_ALGO);

  ret = I2C_SetPointer_Read(VOC_ALO_SIZE);

  // save VOC data
  for (int i = 0; i < VOC_ALO_SIZE; i++) {
    table[i] =_Receive_BUF[i];
  }  

  return(ret);
}

uint8_t SEN55::GetNoxAlgorithm(sen_xox *nox) {
  uint8_t ret;
  
  I2C_fill_buffer(SEN55_NOX_TUNING);
  
  ret = I2C_SetPointer_Read(12);

  nox->IndexOffset  = byte_to_int16_t(0) ;
  nox->LearnTimeOffsetHours  = byte_to_int16_t(2) ;
  nox->LearnTimeGainHours  = byte_to_int16_t(4) ;
  nox->GateMaxDurationMin  = byte_to_int16_t(6) ;
  nox->stdInitial  = byte_to_int16_t(8) ;
  nox->GainFactor  = byte_to_int16_t(10) ;

  return(ret);
}

uint8_t SEN55::GetVocAlgorithm(sen_xox *voc) {
  uint8_t ret;
  
  I2C_fill_buffer(SEN55_VOC_TUNING);
  
  ret = I2C_SetPointer_Read(12);

  voc->IndexOffset  = byte_to_int16_t(0) ;
  voc->LearnTimeOffsetHours  = byte_to_int16_t(2) ;
  voc->LearnTimeGainHours  = byte_to_int16_t(4) ;
  voc->GateMaxDurationMin  = byte_to_int16_t(6) ;
  voc->stdInitial  = byte_to_int16_t(8) ;
  voc->GainFactor  = byte_to_int16_t(10) ;

  return(ret);
}

uint8_t SEN55::SetVocAlgorithmState(uint8_t *table, uint8_t tablesize)
{
  // Voc Algorithm is 8 bytes ( NOT 10 or 11 as in the datasheet)
  if (tablesize < VOC_ALO_SIZE) return(SEN55_ERR_PARAMETER);
  
  I2C_fill_buffer(SEN55_SET_VOC_ALGO, table);

  return(I2C_SetPointer());
}

uint8_t SEN55::SetNoxAlgorithm(sen_xox *nox)
{
  // MUST be values (according to datasheet))
  nox->LearnTimeGainHours = 12;
  nox->stdInitial = 50;
  
  // check limits
  if (nox->IndexOffset > 250 || nox->IndexOffset < 1) nox->IndexOffset = 1;
  if (nox->LearnTimeOffsetHours > 1000 || nox->LearnTimeOffsetHours < 1) nox->LearnTimeOffsetHours = 12;
  if (nox->GateMaxDurationMin > 3000 || nox->GateMaxDurationMin < 1) nox->GateMaxDurationMin = 720;
  if (nox->GainFactor > 1000 || nox->GainFactor < 1) nox->GainFactor = 230;
  
  I2C_fill_buffer(SEN55_SET_NOX_TUNING, nox);
  
  return(I2C_SetPointer());

}

uint8_t SEN55::SetVocAlgorithm(sen_xox *voc)
{
  // check limits (else default according to datasheet)
  if (voc->IndexOffset > 250 || voc->IndexOffset < 1) voc->IndexOffset = 100;
  if (voc->LearnTimeOffsetHours > 1000 || voc->LearnTimeOffsetHours < 1) voc->LearnTimeOffsetHours = 12;
  if (voc->LearnTimeGainHours > 1000 || voc->LearnTimeGainHours < 1) voc->LearnTimeGainHours = 12;
  if (voc->GateMaxDurationMin > 3000 || voc->GateMaxDurationMin < 1) voc->GateMaxDurationMin = 180;
  if (voc->GateMaxDurationMin > 5000 || voc->GateMaxDurationMin < 10) voc->GateMaxDurationMin = 50;
  if (voc->GainFactor > 1000 || voc->GainFactor < 1) voc->GainFactor = 230;
  
  I2C_fill_buffer(SEN55_SET_VOC_TUNING, voc);
  
  return(I2C_SetPointer());
}

uint8_t SEN55::GetTmpComp(sen_tmp_comp *tmp)
{
  uint8_t ret;
  
  I2C_fill_buffer(SEN55_TEMP_COMP);
  
  ret = I2C_SetPointer_Read(6);

  // get values and apply scaling
  tmp->offset = byte_to_int16_t(0) / 200 ;
  tmp->slope  = byte_to_int16_t(2) / 1000;
  tmp->time   = byte_to_Uint16_t(4) ;

  return(ret);
}

uint8_t SEN55::SetTmpComp(sen_tmp_comp *tmp)
{
  // apply scaling
  tmp->offset = tmp->offset * 200;
  tmp->slope = tmp->slope * 1000;

  I2C_fill_buffer(SEN55_SET_TEMP_COMP, tmp);
  
  return(I2C_SetPointer());
}

/**
 * @brief : get error description
 * @param code : error code
 * @param buf  : buffer to store the description
 * @param len  : length of buffer
 */
void SEN55::GetErrDescription(uint8_t code, char *buf, int len)
{

#if defined SMALLFOOTPRINT
  strncpy(buf, "Error-info not disabled", len);
#else
  int i=0;

  while (SEN55_ERR_desc[i].code != 0xff) {
      if(SEN55_ERR_desc[i].code == code)  break;
      i++;
  }

  strncpy(buf, SEN55_ERR_desc[i].desc, len);
#endif // SMALLFOOTPRINT
}

/**
 * @brief : read all values from the sensor and store in structure
 * @param : pointer to structure to store
 *
 * return
 *  SEN55_ERR_OK = ok
 *  else error
 */
uint8_t SEN55::GetValues(struct sen_values *v,  bool laser)
{
  uint8_t ret;

  // measurement started already?
  if ( ! _started ) {
    if (laser) {
      if ( ! start() ) return(SEN55_ERR_CMDSTATE);
    }
    else {
      // NO laser
      if (! startRHTG() ) return(SEN55_ERR_CMDSTATE);
    }
    delay(100);
  }
  
  I2C_fill_buffer(SEN55_READ_MEASURED_VALUE);
  
  ret = I2C_SetPointer_Read(16);

  if (ret != SEN55_ERR_OK) return (ret);

  memset(v,0x0,sizeof(struct sen_values));

  // get data
  if (laser) {
    v->MassPM1 = (float)((byte_to_Uint16_t(0)) / (float) 10);
    v->MassPM2 = (float)((byte_to_Uint16_t(2)) / (float) 10);
    v->MassPM4 = (float)((byte_to_Uint16_t(4)) / (float) 10);
    v->MassPM10 =(float)((byte_to_Uint16_t(6)) / (float) 10);
  }
  v->Hum =  (float)((byte_to_int16_t(8)) / (float) 100);       // Compensated Ambient Humidity [%RH]
  v->Temp = (float)((byte_to_int16_t(10)) / (float) 200);      // Compensated Ambient Temperature [°C]
  v->VOC =  (float)((byte_to_int16_t(12)) / (float) 10);       // VOC Index
  v->NOX =  (float)((byte_to_int16_t(14)) / (float) 10);       // NOx Index
 
  return(SEN55_ERR_OK);
}

/**
 * @brief : read all mass, num and partsize values from the sensor and store in structure
 * 
 * @param v: pointer to structure to store
 *
 * return
 *  SEN55_ERR_OK = ok
 *  else error
 */
uint8_t SEN55::GetValuesPM(struct sen_values_pm *v)
{
  uint8_t ret;

  // measurement started already?
  if ( ! _started ) {
    if ( ! start() ) return(SEN55_ERR_CMDSTATE);
  }

  I2C_fill_buffer(SEN55_READ_MEASURED_VALUE_PM);
  
  ret = I2C_SetPointer_Read(20);

  if (ret != SEN55_ERR_OK) return (ret);

  memset(v,0x0,sizeof(struct sen_values_pm));

  // get data
  v->MassPM1 = (float)((byte_to_Uint16_t(0)) / (float) 10);
  v->MassPM2 = (float)((byte_to_Uint16_t(2)) / (float) 10);
  v->MassPM4 = (float)((byte_to_Uint16_t(4)) / (float) 10);
  v->MassPM10 =(float)((byte_to_Uint16_t(6)) / (float) 10);
  v->NumPM0 =  (float)((byte_to_Uint16_t(8)) / (float) 10);
  v->NumPM1 =  (float)((byte_to_Uint16_t(10)) / (float) 10);
  v->NumPM2 =  (float)((byte_to_Uint16_t(12)) / (float) 10);
  v->NumPM4 =  (float)((byte_to_Uint16_t(14)) / (float) 10);
  v->NumPM10 = (float)((byte_to_Uint16_t(16)) / (float) 10);
  v->PartSize =(float)((byte_to_Uint16_t(18)) / (float) 1000);
 
  return(SEN55_ERR_OK);
}

////////////////// convert routines ///////////////////////////////
/**
 * @brief : translate 4 bytes to Uint32
 * @param x : offset in _Receive_BUF
 *
 * Used for Get Auto Clean interval
 * return : Uint32 number
 */
uint32_t SEN55::byte_to_U32(int x)
{
  ByteToU32_55 conv;

  for (byte i = 0; i < 4; i++){
    conv.array[3-i] = _Receive_BUF[x+i]; //or conv.array[i] = _Receive_BUF[x+i]; depending on endianness
  }

  return conv.value;
}

/**
 * @brief : translate 2 bytes to uint16_t
 * @param x : offset in _Receive_BUF
 *
 * return : uint16_t number
 */
uint16_t SEN55::byte_to_Uint16_t(int x)
{
  uint16_t conv;
  
  conv = _Receive_BUF[x] << 8;
  conv |= _Receive_BUF[x+1];
  
  return conv;
}

/**
 * @brief : translate 2 bytes to int16_t
 * @param x : offset in _Receive_BUF
 *
 * return : uint16_t number
 */
int16_t SEN55::byte_to_int16_t(int x)
{
  int16_t conv;
  
  conv = _Receive_BUF[x] << 8;
  conv |= _Receive_BUF[x+1];
  
  return conv;
}

/************************************************************
 * I2C routines
 *************************************************************/

/**
 * @brief : Start I2C communication from library
 */
void SEN55::I2C_init()
{
  Wire.begin();
  _i2cPort = &Wire;
  _i2cPort->setClock(100000);
}

/**
 * @brief : Fill buffer to send over I2C communication
 * @param cmd: I2C commmand
 * @param interval (optional): value to set for a set-command
 *
 */
void SEN55::I2C_fill_buffer(uint16_t cmd, void *val)
{
  memset(_Send_BUF,0x0,sizeof(_Send_BUF));
  _Send_BUF_Length = 0;
  
  int i = 0;
  
  uint8_t * vv = (uint8_t *) val;
  struct sen_xox * n = (sen_xox *) val;
  struct sen_tmp_comp * t = (sen_tmp_comp *) val;

  switch(cmd) {
  
    case SEN55_SET_AUTO_CLEANING_INTERVAL:

      _Send_BUF[i++] = SEN55_AUTO_CLEANING_INTERVAL >> 8 & 0xff;   //0 MSB
      _Send_BUF[i++] = SEN55_AUTO_CLEANING_INTERVAL & 0xff;        //1 LSB
      _Send_BUF[i++] = data32 >> 24 & 0xff;       //2 MSB
      _Send_BUF[i++] = data32 >> 16 & 0xff;       //3 MSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[2]); //4 CRC
      _Send_BUF[i++] = data32 >>8 & 0xff;         //5 LSB
      _Send_BUF[i++] = data32 & 0xff;             //6 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[5]); //7 CRC
      break;
    
    case SEN55_SET_VOC_ALGO:
 
      _Send_BUF[i++] = SEN55_VOC_ALGO >> 8 & 0xff;   //0 MSB
      _Send_BUF[i++] = SEN55_VOC_ALGO & 0xff;        //1 LSB
      
      for (int j = 0, k = 0 ; j < VOC_ALO_SIZE;) {
        _Send_BUF[i++] = vv[j++] & 0xff;

        if (k++ > 2) {
          uint8_t st = i - j;
          _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[st]); //CRC
          k = 0;
        }
      }
      break;
      
    case SEN55_SET_NOX_TUNING:
      _Send_BUF[i++] = SEN55_NOX_TUNING >> 8 & 0xff;   //0 MSB
      _Send_BUF[i++] = SEN55_NOX_TUNING & 0xff;        //1 LSB    
    
      // add data
      _Send_BUF[i++] = n->IndexOffset >>8 & 0xff;          //2 MSB
      _Send_BUF[i++] = n->IndexOffset & 0xff;              //3 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[2]); //4 CRC
      _Send_BUF[i++] = n->LearnTimeOffsetHours >>8 & 0xff; //5 MSB
      _Send_BUF[i++] = n->LearnTimeOffsetHours & 0xff;     //6 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[5]); //7 CRC
      _Send_BUF[i++] = n->LearnTimeGainHours >>8 & 0xff;   //8 MSB
      _Send_BUF[i++] = n->LearnTimeGainHours & 0xff;       //9 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[8]); //10 CRC
      _Send_BUF[i++] = n->GateMaxDurationMin >>8 & 0xff;   //11 MSB
      _Send_BUF[i++] = n->GateMaxDurationMin & 0xff;       //12 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[11]); //13 CRC
      _Send_BUF[i++] = n->stdInitial >>8 & 0xff;           //14 MSB
      _Send_BUF[i++] = n->stdInitial & 0xff;               //15 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[14]); //16 CRC
      _Send_BUF[i++] = n->GainFactor >>8 & 0xff;           //17 MSB
      _Send_BUF[i++] = n->GainFactor & 0xff;               //18 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[17]); //19 CRC
      break;

    case SEN55_SET_VOC_TUNING:
      _Send_BUF[i++] = SEN55_VOC_TUNING >> 8 & 0xff;   //0 MSB
      _Send_BUF[i++] = SEN55_VOC_TUNING & 0xff;        //1 LSB    
    
      // add data
      _Send_BUF[i++] = n->IndexOffset >>8 & 0xff;          //2 MSB
      _Send_BUF[i++] = n->IndexOffset & 0xff;              //3 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[2]); //4 CRC
      _Send_BUF[i++] = n->LearnTimeOffsetHours >>8 & 0xff; //5 MSB
      _Send_BUF[i++] = n->LearnTimeOffsetHours & 0xff;     //6 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[5]); //7 CRC
      _Send_BUF[i++] = n->LearnTimeGainHours >>8 & 0xff;   //8 MSB
      _Send_BUF[i++] = n->LearnTimeGainHours & 0xff;       //9 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[8]); //10 CRC
      _Send_BUF[i++] = n->GateMaxDurationMin >>8 & 0xff;   //11 MSB
      _Send_BUF[i++] = n->GateMaxDurationMin & 0xff;       //12 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[11]); //13 CRC
      _Send_BUF[i++] = n->stdInitial >>8 & 0xff;           //14 MSB
      _Send_BUF[i++] = n->stdInitial & 0xff;               //15 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[14]); //16 CRC
      _Send_BUF[i++] = n->GainFactor >>8 & 0xff;           //17 MSB
      _Send_BUF[i++] = n->GainFactor & 0xff;               //18 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[17]); //19 CRC
      break;      
      
    case SEN55_SET_TEMP_COMP:
      _Send_BUF[i++] = SEN55_TEMP_COMP >> 8 & 0xff;   //0 MSB
      _Send_BUF[i++] = SEN55_TEMP_COMP & 0xff;        //1 LSB
        
       // add data
      _Send_BUF[i++] = t->offset >>8 & 0xff;          //2 MSB
      _Send_BUF[i++] = t->offset & 0xff;              //3 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[2]); //4 CRC
      _Send_BUF[i++] = t->slope >>8 & 0xff;           //5 MSB
      _Send_BUF[i++] = t->slope & 0xff;               //6 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[5]); //7 CRC
      _Send_BUF[i++] = t->time >>8 & 0xff;            //8 MSB
      _Send_BUF[i++] = t->time & 0xff;                //9 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[8]); //10 CRC
      break;
      
    case SEN55_SET_WARM_START_PARAM:
      _Send_BUF[i++] = SEN55_WARM_START_PARAM >> 8 & 0xff;   //0 MSB
      _Send_BUF[i++] = SEN55_WARM_START_PARAM & 0xff;        //1 LSB
        
       // add data
      _Send_BUF[i++] = data16 >>8 & 0xff;          //2 MSB
      _Send_BUF[i++] = data16 & 0xff;              //3 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[2]); //4 CRC 
      break;
    
    case SEN55_SET_RHT_ACCEL:
      _Send_BUF[i++] = SEN55_RHT_ACCEL >> 8 & 0xff;   //0 MSB
      _Send_BUF[i++] = SEN55_RHT_ACCEL & 0xff;        //1 LSB
        
       // add data
      _Send_BUF[i++] = data16 >>8 & 0xff;          //2 MSB
      _Send_BUF[i++] = data16 & 0xff;              //3 LSB
      _Send_BUF[i++] = I2C_calc_CRC(&_Send_BUF[2]); //4 CRC 
      break;
      
    default:
      // add command
      _Send_BUF[i++] = cmd >> 8 & 0xff;   //0 MSB
      _Send_BUF[i++] = cmd & 0xff;        //1 LSB
      break;
  }

 _Send_BUF_Length = i;
}

/**
 * @brief : SetPointer (and write data if included) with I2C communication
 *
 * return:
 * Ok SEN55_ERR_OK
 * else error
 */
uint8_t SEN55::I2C_SetPointer()
{
  if (_Send_BUF_Length == 0) return(SEN55_ERR_DATALENGTH);

  if (_SEN55_Debug) {
    DebugPrintf("I2C Sending: ");
    for(byte i = 0; i < _Send_BUF_Length; i++)
      DebugPrintf(" 0x%02X", _Send_BUF[i]);
    DebugPrintf("\n");
  }

  _i2cPort->beginTransmission(SEN55_ADDRESS);
  _i2cPort->write(_Send_BUF, _Send_BUF_Length);
  _i2cPort->endTransmission();

  return(SEN55_ERR_OK);
}

/**
 * @brief : read with I2C communication
 * @param cnt: number of data bytes to get
 * @param chk_zero : needed for read info buffer
 *  false : expect all the bytes
 *  true  : expect NULL termination and cnt is MAXIMUM byte
 *
 */
uint8_t SEN55::I2C_SetPointer_Read(uint8_t cnt, bool chk_zero)
{
  uint8_t ret;

  // set pointer
  ret = I2C_SetPointer();
  
  if (ret != SEN55_ERR_OK) {
    DebugPrintf("Can not set pointer\n");
    return(ret);
  }
  
  // could not get it to work on UNOR4 without this delay.
  delay(5);
  
  // read from Sensor
  ret = I2C_ReadToBuffer(cnt, chk_zero);

  if (_SEN55_Debug) {
    DebugPrintf("I2C Received: ");
    for(byte i = 0; i < _Receive_BUF_Length; i++)
      DebugPrintf("0x%02X ",_Receive_BUF[i]);
    DebugPrintf("length: %d\n\n",_Receive_BUF_Length);
  }

  if (ret != SEN55_ERR_OK) {
    DebugPrintf("Error during reading from I2C: 0x%02X\n", ret);
  }
  
  return(ret);
}

/**
 * @brief       : receive from Sensor with I2C communication
 * @param count : number of data bytes to expect
 * @param chk_zero :  check for zero termination ( Serial and product code)
 *  false : expect and rea all the data bytes
 *  true  : expect NULL termination and count is MAXIMUM data bytes
 *
 * return :
 * OK   SEN55_ERR_OK
 * else error
 */
uint8_t SEN55::I2C_ReadToBuffer(uint8_t count, bool chk_zero)
{
  uint8_t data[3];
  uint8_t i, j, exp_cnt, rec_cnt;

  j = i = _Receive_BUF_Length = 0;
  
  // 2 data bytes  + crc
  exp_cnt = count / 2 * 3;
  
// in case of AVR expect small wire buffer
// this will only impact reading serial number and name
#ifdef MAX_32_TO_EXPECT
  if (exp_cnt > 32) exp_cnt = 32;
#endif

  rec_cnt = _i2cPort->requestFrom((uint8_t) SEN55_ADDRESS, exp_cnt);

  if (rec_cnt != exp_cnt ){
    DebugPrintf("Did not receive all bytes: Expected 0x%02X, got 0x%02X\n",exp_cnt & 0xff,rec_cnt & 0xff);
    return(SEN55_ERR_PROTOCOL);
  }
  
  while (_i2cPort->available()) {  // read all

    data[i++] = _i2cPort->read();
//DebugPrintf("data 0x%02X\n", data[i-1]);
    // 2 bytes RH, 1 CRC
    if( i == 3) {

      if (data[2] != I2C_calc_CRC(&data[0])){
        DebugPrintf("I2C CRC error: Expected 0x%02X, calculated 0x%02X\n",data[2] & 0xff,I2C_calc_CRC(&data[0]) & 0xff);
        return(SEN55_ERR_PROTOCOL);
      }

      _Receive_BUF[_Receive_BUF_Length++] = data[0];
      _Receive_BUF[_Receive_BUF_Length++] = data[1];

      i = 0;

      // check for zero termination (Serial and product code)
      if (chk_zero) {

        if (data[0] == 0 && data[1] == 0) {

          // flush any bytes pending (if NOT clearing rxBuffer)
          // Logged as an issue and expect this could be removed in the future
          while (_i2cPort->available()) _i2cPort->read();
          return(SEN55_ERR_OK);
        }
      }

      if (_Receive_BUF_Length >= count) break;
    }
  }

  if (i != 0) {
    DebugPrintf("Error: Data counter %d\n",i);
    while (j < i) _Receive_BUF[_Receive_BUF_Length++] = data[j++];
  }

  if (_Receive_BUF_Length == 0) {
    DebugPrintf("Error: Received NO bytes\n");
    return(SEN55_ERR_PROTOCOL);
  }

  if (_Receive_BUF_Length == count) return(SEN55_ERR_OK);

  DebugPrintf("Error: Expected bytes : %d, Received bytes %d\n", count,_Receive_BUF_Length);

  return(SEN55_ERR_DATALENGTH);
}

/**
 * @brief :check for data ready
 *
 * Return
 *  true  if available
 *  false if not
 */
bool SEN55::Check_data_ready()
{
  I2C_fill_buffer(SEN55_READ_DATA_RDY_FLAG);
  
  if (I2C_SetPointer_Read(2) != SEN55_ERR_OK) return(false);
  
  if (_Receive_BUF[1] == 1) return(true);
  
  return(false);
}

/**
 * @brief : calculate CRC for I2c comms
 * @param data : 2 databytes to calculate the CRC from
 *
 * Source : datasheet SEN55
 *
 * return CRC
 */
uint8_t SEN55::I2C_calc_CRC(uint8_t data[2])
{
  uint8_t crc = 0xFF;
  for(int i = 0; i < 2; i++) {
    crc ^= data[i];
    for(uint8_t bit = 8; bit > 0; --bit) {
      if(crc & 0x80) {
        crc = (crc << 1) ^ 0x31u;
      } else {
        crc = (crc << 1);
      }
    }
  }

  return crc;
}
