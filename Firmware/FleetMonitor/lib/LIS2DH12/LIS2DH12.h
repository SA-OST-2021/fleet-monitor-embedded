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

#ifndef LIS2DH12_H
#define LIS2DH12_H

#include <Arduino.h>
#include <SPI.h>

#include "lis2dh12_reg.h" //This is the ST library

class LIS2DH12
{
public:
  uint8_t begin(uint8_t sspin, uint8_t clkpin, uint8_t misopin, uint8_t mosipin); //Begin comm with accel at given I2C address, and given wire port
  uint8_t isConnected();                                                        //Returns true if an accel sensor is detected at library's I2C address
  bool available();                                                          //Returns true if new accel data is available
  void waitForNewData();                                                     //Block until new data is available
  bool temperatureAvailable();                                               //Returns true if new temp data is available

  uint8_t readReg(uint8_t reg);

  float getX(); //Return latest accel data in milli-g's. If data has already be read, initiate new read.
  float getY();
  float getZ();
  int16_t getRawX(); //Return raw 16 bit accel reading
  int16_t getRawY();
  int16_t getRawZ();
  float getTemperature(); //Returns latest temp data in C. If data is old, initiate new read.

  void parseAccelData(); //Load sensor data into global vars. Call after new data is avaiable.
  void getTempData();

  void enableTemperature();  //Enable the onboard temp sensor
  void disableTemperature(); //Disable the onboard temp sensor

  void setDataRate(uint8_t dataRate); //Set the output data rate of sensor. Higher rates consume more current.
  uint8_t getDataRate();              //Returns the output data rate of sensor.

  void setScale(uint8_t scale); //Set full scale: +/-2, 4, 8, or 16g
  uint8_t getScale();           //Returns current scale of sensor

  void setMode(uint8_t mode); //Set mode to low, normal, or high data rate
  uint8_t getMode();          //Get current sensor mode

  void enableSelfTest(bool direction = true);
  void disableSelfTest();

  void enableTapDetection(); //Enable the single tap interrupt
  void disableTapDetection();
  void setTapThreshold(uint8_t threshold); //Set the 7-bit threshold value for tap and double tap
  bool isTapped();                         //Returns true if Z, Y, or X tap detection bits are set

  lis2dh12_ctx_t dev_ctx;

  static int32_t platform_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
  static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
  
  

private:
  bool xIsFresh = false;
  bool yIsFresh = false;
  bool zIsFresh = false;
  bool tempIsFresh = false;

  uint8_t currentScale = 0; //Needed to convert readings to mg
  uint8_t currentMode = 0;  //Needed to convert readings to mg

  float accelX;
  float accelY;
  float accelZ;
  uint16_t rawX;
  uint16_t rawY;
  uint16_t rawZ;
  float temperatureC;


};

#endif /* LIS2DH12_H */
