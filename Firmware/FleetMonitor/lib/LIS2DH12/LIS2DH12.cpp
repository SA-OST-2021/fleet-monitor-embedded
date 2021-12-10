/*
  This is a library written for the SparkFun LIS2DH12
  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/15420

  Written by Nathan Seidle @ SparkFun Electronics, September 21st, 2019

  The LIS2DH12 is a very low power I2C triple axis accelerometer
  The SparkFun LIS2DH12 library is merely a wrapper for the ST library. Please
  see the lis2dh12_reg files for licensing and portable C functions.

  https://github.com/sparkfun/LIS2DH12_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.9

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "LIS2DH12.h"
#include "Arduino.h"

#define SPI_ACC_SETTINGS SPISettings(2000000, MSBFIRST, SPI_MODE0)

SPIClass SPI_Port;
uint8_t sspin;

//Begin comm with accel at given I2C address, and given wire port
//Init accel with default settings
uint8_t LIS2DH12::begin(uint8_t cspin, uint8_t clkpin, uint8_t misopin, uint8_t mosipin)
{
  SPI_Port.begin(clkpin, misopin, mosipin);
  pinMode(cspin, OUTPUT);
  digitalWrite(cspin, HIGH);

  sspin = cspin;

  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = (void *)this;

  if (isConnected() != LIS2DH12_ID)
    return false;

  uint8_t reset = 0x00;
  platform_write(this, LIS2DH12_CTRL_REG3, &reset, 1);
  platform_write(this, LIS2DH12_CTRL_REG4, &reset, 1);

  //Disable Block Data Update
  lis2dh12_block_data_update_set(&dev_ctx, PROPERTY_DISABLE);

  //Set Output Data Rate to 25Hz
  setDataRate(LIS2DH12_ODR_50Hz);

  //Set full scale to 2g
  setScale(LIS2DH12_2g);

  //Enable temperature sensor
  enableTemperature();

  //Set device in continuous mode with 12 bit resol.
  setMode(LIS2DH12_HR_12bit);

  return true;
}

//Check to see if IC ack its I2C address. Then check for valid LIS2DH ID.
uint8_t LIS2DH12::isConnected()
{
  uint8_t buffer;
  platform_read(this, LIS2DH12_WHO_AM_I, &buffer, 1);
  return buffer;
}

uint8_t LIS2DH12::readReg(uint8_t reg){
  uint8_t buffer;
  platform_read(this, reg, &buffer, 1);
  return buffer;
}

//Returns true if new data is available
bool LIS2DH12::available()
{
  lis2dh12_reg_t reg;
  lis2dh12_xl_data_ready_get(&dev_ctx, &reg.byte);
  if (reg.byte)
    return true;
  return false;
}

//Blocking wait until new data is available
void LIS2DH12::waitForNewData()
{
  while (available() == false)
    delay(1);
}

//Returns true if new temperature data is available
bool LIS2DH12::temperatureAvailable()
{
  lis2dh12_reg_t reg;
  lis2dh12_temp_data_ready_get(&dev_ctx, &reg.byte);
  if (reg.byte)
    return true;
  return false;
}

//Returns X accel of the global accel data
float LIS2DH12::getX()
{
  if (xIsFresh == false)
  {
    waitForNewData(); //Blocking wait until available
    parseAccelData();
  }
  xIsFresh = false;
  return (accelX);
}

//Returns X of the global accel data
int16_t LIS2DH12::getRawX()
{
  if (xIsFresh == false)
  {
    waitForNewData(); //Blocking wait until available
    parseAccelData();
  }
  xIsFresh = false;
  return (rawX);
}

//Returns Y accel of the global accel data
float LIS2DH12::getY()
{
  if (yIsFresh == false)
  {
    waitForNewData(); //Blocking wait until available
    parseAccelData();
  }
  yIsFresh = false;
  return (accelY);
}

//Returns Y of the global accel data
int16_t LIS2DH12::getRawY()
{
  if (yIsFresh == false)
  {
    waitForNewData(); //Blocking wait until available
    parseAccelData();
  }
  yIsFresh = false;
  return (rawY);
}

//Returns Z accel of the global accel data
float LIS2DH12::getZ()
{
  if (zIsFresh == false)
  {
    waitForNewData(); //Blocking wait until available
    parseAccelData();
  }
  zIsFresh = false;
  return (accelZ);
}

//Returns Z of the global accel data
int16_t LIS2DH12::getRawZ()
{
  if (zIsFresh == false)
  {
    waitForNewData(); //Blocking wait until available
    parseAccelData();
  }
  zIsFresh = false;
  return (rawZ);
}

//Returns sensor temperature in C
float LIS2DH12::getTemperature()
{
  if (tempIsFresh == false)
  {
    waitForNewData(); //Blocking wait until available
    getTempData();
  }
  tempIsFresh = false;
  return (temperatureC);
}

//Load global vars with latest accel data
//Does not guarantee data is fresh (ie you can read the same accel values multiple times)
void LIS2DH12::parseAccelData()
{
  // Read accelerometer data
  axis3bit16_t data_raw_acceleration;
  memset(data_raw_acceleration.u8bit, 0x00, 3 * sizeof(int16_t));
  lis2dh12_acceleration_raw_get(&dev_ctx, data_raw_acceleration.u8bit);

  rawX = data_raw_acceleration.i16bit[0];
  rawY = data_raw_acceleration.i16bit[1];
  rawZ = data_raw_acceleration.i16bit[2];

  //Convert the raw accel data into milli-g's based on current scale and mode
  switch (currentScale)
  {
  case LIS2DH12_2g:
    switch (currentMode)
    {
    case LIS2DH12_HR_12bit: //High resolution
      accelX = lis2dh12_from_fs2_hr_to_mg(rawX);
      accelY = lis2dh12_from_fs2_hr_to_mg(rawY);
      accelZ = lis2dh12_from_fs2_hr_to_mg(rawZ);
      break;
    case LIS2DH12_NM_10bit: //Normal mode
      accelX = lis2dh12_from_fs2_nm_to_mg(rawX);
      accelY = lis2dh12_from_fs2_nm_to_mg(rawY);
      accelZ = lis2dh12_from_fs2_nm_to_mg(rawZ);
      break;
    case LIS2DH12_LP_8bit: //Low power mode
      accelX = lis2dh12_from_fs2_lp_to_mg(rawX);
      accelY = lis2dh12_from_fs2_lp_to_mg(rawY);
      accelZ = lis2dh12_from_fs2_lp_to_mg(rawZ);
      break;
    }
    break;

  case LIS2DH12_4g:
    switch (currentMode)
    {
    case LIS2DH12_HR_12bit: //High resolution
      accelX = lis2dh12_from_fs4_hr_to_mg(rawX);
      accelY = lis2dh12_from_fs4_hr_to_mg(rawY);
      accelZ = lis2dh12_from_fs4_hr_to_mg(rawZ);
      break;
    case LIS2DH12_NM_10bit: //Normal mode
      accelX = lis2dh12_from_fs4_nm_to_mg(rawX);
      accelY = lis2dh12_from_fs4_nm_to_mg(rawY);
      accelZ = lis2dh12_from_fs4_nm_to_mg(rawZ);
      break;
    case LIS2DH12_LP_8bit: //Low power mode
      accelX = lis2dh12_from_fs4_lp_to_mg(rawX);
      accelY = lis2dh12_from_fs4_lp_to_mg(rawY);
      accelZ = lis2dh12_from_fs4_lp_to_mg(rawZ);
      break;
    }
    break;

  case LIS2DH12_8g:
    switch (currentMode)
    {
    case LIS2DH12_HR_12bit: //High resolution
      accelX = lis2dh12_from_fs8_hr_to_mg(rawX);
      accelY = lis2dh12_from_fs8_hr_to_mg(rawY);
      accelZ = lis2dh12_from_fs8_hr_to_mg(rawZ);
      break;
    case LIS2DH12_NM_10bit: //Normal mode
      accelX = lis2dh12_from_fs8_nm_to_mg(rawX);
      accelY = lis2dh12_from_fs8_nm_to_mg(rawY);
      accelZ = lis2dh12_from_fs8_nm_to_mg(rawZ);
      break;
    case LIS2DH12_LP_8bit: //Low power mode
      accelX = lis2dh12_from_fs8_lp_to_mg(rawX);
      accelY = lis2dh12_from_fs8_lp_to_mg(rawY);
      accelZ = lis2dh12_from_fs8_lp_to_mg(rawZ);
      break;
    }
    break;

  case LIS2DH12_16g:
    switch (currentMode)
    {
    case LIS2DH12_HR_12bit: //High resolution
      accelX = lis2dh12_from_fs16_hr_to_mg(rawX);
      accelY = lis2dh12_from_fs16_hr_to_mg(rawY);
      accelZ = lis2dh12_from_fs16_hr_to_mg(rawZ);
      break;
    case LIS2DH12_NM_10bit: //Normal mode
      accelX = lis2dh12_from_fs16_nm_to_mg(rawX);
      accelY = lis2dh12_from_fs16_nm_to_mg(rawY);
      accelZ = lis2dh12_from_fs16_nm_to_mg(rawZ);
      break;
    case LIS2DH12_LP_8bit: //Low power mode
      accelX = lis2dh12_from_fs16_lp_to_mg(rawX);
      accelY = lis2dh12_from_fs16_lp_to_mg(rawY);
      accelZ = lis2dh12_from_fs16_lp_to_mg(rawZ);
      break;
    }
    break;

  default: //2g
    accelX = lis2dh12_from_fs2_hr_to_mg(rawX);
    accelY = lis2dh12_from_fs2_hr_to_mg(rawY);
    accelZ = lis2dh12_from_fs2_hr_to_mg(rawZ);
    break;
  }

  xIsFresh = true;
  yIsFresh = true;
  zIsFresh = true;
}

//Load global vars with latest temp data
//Does not guarantee data is fresh (ie you can read the same temp value multiple times)
void LIS2DH12::getTempData()
{
  //Read temperature data
  axis1bit16_t data_raw_temperature;
  memset(data_raw_temperature.u8bit, 0x00, sizeof(int16_t));
  lis2dh12_temperature_raw_get(&dev_ctx, data_raw_temperature.u8bit);

  switch (currentMode)
  {
  case LIS2DH12_HR_12bit: //High resolution
    temperatureC = lis2dh12_from_lsb_hr_to_celsius(data_raw_temperature.i16bit);
    break;
  case LIS2DH12_NM_10bit: //Normal mode
    temperatureC = lis2dh12_from_lsb_nm_to_celsius(data_raw_temperature.i16bit);
    break;
  case LIS2DH12_LP_8bit: //Low power mode
    temperatureC = lis2dh12_from_lsb_lp_to_celsius(data_raw_temperature.i16bit);
    break;
  }

  tempIsFresh = true;
}

//Enter a self test
void LIS2DH12::enableSelfTest(bool direction)
{
  if (direction == true)
  {
    lis2dh12_self_test_set(&dev_ctx, LIS2DH12_ST_POSITIVE);
  }
  else
  {
    lis2dh12_self_test_set(&dev_ctx, LIS2DH12_ST_NEGATIVE);
  }
}

//Exit self test
void LIS2DH12::disableSelfTest()
{
  lis2dh12_self_test_set(&dev_ctx, LIS2DH12_ST_DISABLE);
}

//Set the output data rate of the sensor
void LIS2DH12::setDataRate(uint8_t dataRate)
{
  if (dataRate > LIS2DH12_ODR_5kHz376_LP_1kHz344_NM_HP)
    dataRate = LIS2DH12_ODR_25Hz; //Default to 25Hz
  lis2dh12_data_rate_set(&dev_ctx, (lis2dh12_odr_t)dataRate);
}

//Return the output data rate of the sensor
uint8_t LIS2DH12::getDataRate(void)
{
  lis2dh12_odr_t dataRate;
  lis2dh12_data_rate_get(&dev_ctx, &dataRate);
  return ((uint8_t)dataRate);
}

//Set full scale of output to +/-2, 4, 8, or 16g
void LIS2DH12::setScale(uint8_t scale)
{
  if (scale > LIS2DH12_16g)
    scale = LIS2DH12_2g; //Default to LIS2DH12_2g

  currentScale = scale; //Used for mg conversion in getX/Y/Z functions

  lis2dh12_full_scale_set(&dev_ctx, (lis2dh12_fs_t)scale);
}

//Return the current scale of the sensor
uint8_t LIS2DH12::getScale(void)
{
  lis2dh12_fs_t scale;
  lis2dh12_full_scale_get(&dev_ctx, &scale);
  return ((uint8_t)scale);
}

//Enable the onboard temperature sensor
void LIS2DH12::enableTemperature()
{
  lis2dh12_temperature_meas_set(&dev_ctx, LIS2DH12_TEMP_ENABLE);
}

//Anti-Enable the onboard temperature sensor
void LIS2DH12::disableTemperature()
{
  lis2dh12_temperature_meas_set(&dev_ctx, LIS2DH12_TEMP_DISABLE);
}

void LIS2DH12::setMode(uint8_t mode)
{
  if (mode > LIS2DH12_LP_8bit)
    mode = LIS2DH12_HR_12bit; //Default to 12 bit

  currentMode = mode;

  lis2dh12_operating_mode_set(&dev_ctx, (lis2dh12_op_md_t)mode);
}

//Return the current mode of the sensor
uint8_t LIS2DH12::getMode(void)
{
  lis2dh12_op_md_t mode;
  lis2dh12_operating_mode_get(&dev_ctx, &mode);
  return ((uint8_t)mode);
}

//Enable single tap detection
void LIS2DH12::enableTapDetection()
{
  lis2dh12_click_cfg_t newBits;
  if (lis2dh12_tap_conf_get(&dev_ctx, &newBits) == 0)
  {
    newBits.xs = true;
    newBits.ys = true;
    newBits.zs = true;
    lis2dh12_tap_conf_set(&dev_ctx, &newBits);
  }
}

//Disable single tap detection
void LIS2DH12::disableTapDetection()
{
  lis2dh12_click_cfg_t newBits;
  if (lis2dh12_tap_conf_get(&dev_ctx, &newBits) == 0)
  {
    newBits.xs = false;
    newBits.ys = false;
    newBits.zs = false;
    lis2dh12_tap_conf_set(&dev_ctx, &newBits);
  }
}

//Set 7 bit threshold value
void LIS2DH12::setTapThreshold(uint8_t threshold)
{
  if (threshold > 127) //Register is 7 bits wide
    threshold = 127;
  lis2dh12_tap_threshold_set(&dev_ctx, threshold);
}

//Returns true if a tap is detected
bool LIS2DH12::isTapped(void)
{
  lis2dh12_click_src_t interruptSource;
  lis2dh12_tap_source_get(&dev_ctx, &interruptSource);
  if (interruptSource.x || interruptSource.y || interruptSource.z) //Check if ZYX bits are set
  {
    return (true);
  }
  return (false);
}

/*
   @brief  Write generic device register (platform dependent)

   @param  handle    customizable argument. In this examples is used in
                     order to select the correct sensor bus handler.
   @param  reg       register to write
   @param  bufp      pointer to data to write in register reg
   @param  len       number of consecutive register to write

*/
int32_t LIS2DH12::platform_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
  LIS2DH12 *classPointer = (LIS2DH12 *)handle;

  if (len > 30)
  {
    return 1; //Error
  }
  digitalWrite(sspin, LOW);
  SPI_Port.write(reg);
  for (uint16_t x = 0; x < len; x++)
  {
    SPI_Port.write(bufp[x]);
  }

  SPI_Port.endTransaction();
  digitalWrite(sspin, HIGH);
  return 0; //Will return 0 upon success
}

/*
   @brief  Read generic device register (platform dependent)

   @param  handle    customizable argument. In this examples is used in
                     order to select the correct sensor bus handler.
   @param  reg       register to read
   @param  bufp      pointer to buffer that store the data read
   @param  len       number of consecutive register to read

*/
int32_t LIS2DH12::platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
  LIS2DH12 *classPointer = (LIS2DH12 *)handle;
  //static uint8_t tmp[16];
  reg |= 0x80;
  if(len > 1){
    reg |= 0x40;
  }
  digitalWrite(sspin, LOW);
  SPI_Port.beginTransaction(SPI_ACC_SETTINGS);
  SPI_Port.write(reg);
  SPI_Port.transferBytes(NULL, bufp, len);
  SPI_Port.endTransaction();
  digitalWrite(sspin, HIGH);
  return (0); //Success
}
