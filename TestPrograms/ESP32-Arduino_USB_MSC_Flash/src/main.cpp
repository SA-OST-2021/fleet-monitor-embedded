#include <Arduino.h>
#include "USB.h"

#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"

#include "ff.h"
#include "diskio.h"

#define LED_BUILTIN 5

#define DISK_LABEL    "EXT FLASH"


int32_t msc_read_cb (uint32_t lba, void* buffer, uint32_t bufsize);
int32_t msc_write_cb (uint32_t lba, uint8_t* buffer, uint32_t bufsize);
void msc_flush_cb (void);

USBCDC USBSerial;
Adafruit_USBD_MSC usb_msc;
Adafruit_FlashTransport_ESP32 flashTransport;
Adafruit_SPIFlash flash(&flashTransport);

// file system object from SdFat
FatFileSystem fatfs;

FatFile root;
FatFile file;

FATFS elmchamFatfs;
uint8_t workbuf[4096]; // Working buffer for f_fdisk function.


#define BUFSIZE   4096

// 4 byte aligned buffer has best result with nRF QSPI
uint8_t bufwrite[BUFSIZE] __attribute__ ((aligned(4)));
uint8_t bufread[BUFSIZE] __attribute__ ((aligned(4)));



// Set to true when PC write to flash
bool fs_changed;



void print_speed(const char* text, uint32_t count, uint32_t ms)
{
  USBSerial.print(text);
  USBSerial.print(count);
  USBSerial.print(" bytes in ");
  USBSerial.print(ms / 1000.0F, 2);
  USBSerial.println(" seconds.");

  USBSerial.print("Speed : ");
  USBSerial.print( (count / 1000.0F) / (ms / 1000.0F), 2);
  USBSerial.println(" KB/s.\r\n");
}

bool write_and_compare(uint8_t pattern)
{
  uint32_t ms;

  USBSerial.println("Erase chip");
  USBSerial.flush();

#define TEST_WHOLE_CHIP

#ifdef TEST_WHOLE_CHIP
  uint32_t const flash_sz = flash.size();
  flash.eraseChip();
#else
  uint32_t const flash_sz = 4096;
  flash.eraseSector(0);
#endif

  flash.waitUntilReady();

  // write all
  memset(bufwrite, (int) pattern, sizeof(bufwrite));
  USBSerial.printf("Write flash with 0x%02X\n", pattern);
  USBSerial.flush();
  ms = millis();

  for(uint32_t addr = 0; addr < flash_sz; addr += sizeof(bufwrite))
  {
    flash.writeBuffer(addr, bufwrite, sizeof(bufwrite));
  }

  uint32_t ms_write = millis() - ms;
  print_speed("Write ", flash_sz, ms_write);
  USBSerial.flush();

  // read and compare
  USBSerial.println("Read flash and compare");
  USBSerial.flush();
  uint32_t ms_read = 0;
  for(uint32_t addr = 0; addr < flash_sz; addr += sizeof(bufread))
  {
    memset(bufread, 0, sizeof(bufread));

    ms = millis();
    flash.readBuffer(addr, bufread, sizeof(bufread));
    ms_read += millis() - ms;

    if ( memcmp(bufwrite, bufread, BUFSIZE) )
    {
      USBSerial.printf("Error: flash contents mismatched at address 0x%08X!!!", addr);
      for(uint32_t i=0; i<sizeof(bufread); i++)
      {
        if ( i != 0 ) Serial.print(' ');
        if ( (i%16 == 0) )
        {
          USBSerial.println();
          USBSerial.printf("%03X: ", i);
        }

        USBSerial.printf("%02X", bufread[i]);
      }

      USBSerial.println();
      return false;
    }
  }

  print_speed("Read  ", flash_sz, ms_read);
  USBSerial.flush();

  return true;
}






// the setup function runs once when you press reset or power the board
void setup()
{ 
#if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
  // Manual begin() is required on core without built-in support for TinyUSB such as
  // - mbed rp2040
  TinyUSB_Device_Init(0);
#endif

  pinMode(LED_BUILTIN, OUTPUT);

  

  //USB.enableDFU();
  USB.begin();
  USB.productName("ESP32S2-USB");
  USBSerial.begin(115200);
  while (!USBSerial) delay(100);


  Serial.begin(115200);
  delay(100);
  Serial.println("Test Serial");
  USBSerial.println("Test Florian");


  if (!flash.begin()) {
    USBSerial.println("Error, failed to initialize flash chip!");
    while(1) yield();
  }
  /*
  USBSerial.print("Flash chip JEDEC ID: 0x"); USBSerial.println(flash.getJEDECID(), HEX);
  USBSerial.print("Flash size: "); USBSerial.print(flash.size() / 1024); USBSerial.println(" KB");

  flashTransport.setClockSpeed(1*1000000UL, 1*1000000UL);

  flash.setIndicator(LED_BUILTIN, true);

  USBSerial.println("Adafruit Serial Flash Speed Test example");
  USBSerial.print("JEDEC ID: "); USBSerial.println(flash.getJEDECID(), HEX);
  USBSerial.print("Flash size: "); USBSerial.println(flash.size());
  USBSerial.flush();

  write_and_compare(0xAA);
  write_and_compare(0x55);

  USBSerial.println("Speed test is completed.");
  USBSerial.flush();

  */


  /*
  if (!flash.eraseChip()) {
    USBSerial.println("Failed to erase chip!");
  }

  flash.waitUntilReady();
  USBSerial.println("Successfully erased chip!");
  */


 /*


  // Partition the flash with 1 partition that takes the entire space.
  USBSerial.println("Partitioning flash with 1 primary partition...");
  DWORD plist[] = {100, 0, 0, 0};  // 1 primary partition with 100% of space.
  uint8_t buf[512] = {0};          // Working buffer for f_fdisk function.
  FRESULT r = f_fdisk(0, plist, buf);
  if (r != FR_OK) {
    USBSerial.print("Error, f_fdisk failed with error code: "); USBSerial.println(r, DEC);
    while(1);
  }
  USBSerial.println("Partitioned flash!");


  // Call fatfs begin and passed flash object to initialize file system
  USBSerial.println("Creating and formatting FAT filesystem (this takes ~60 seconds)...");



  // Make filesystem.
  r = f_mkfs("", FM_FAT | FM_SFD, 0, workbuf, sizeof(workbuf));
  if (r != FR_OK) {
    USBSerial.print("Error, f_mkfs failed with error code: "); USBSerial.println(r, DEC);
    while(1) yield();
  }


  // mount to set disk label
  r = f_mount(&elmchamFatfs, "0:", 1);
  if (r != FR_OK) {
    USBSerial.print("Error, f_mount failed with error code: "); USBSerial.println(r, DEC);
    while(1) yield();
  }

  // Setting label
  USBSerial.println("Setting disk label to: " DISK_LABEL);
  r = f_setlabel(DISK_LABEL);
  if (r != FR_OK) {
    USBSerial.print("Error, f_setlabel failed with error code: "); USBSerial.println(r, DEC);
    while(1) yield();
  }

  // unmount
  f_unmount("0:");

  // sync to make sure all data is written to flash
  flash.syncBlocks();
  
  USBSerial.println("Formatted flash!");

  // Check new filesystem
  if (!fatfs.begin(&flash)) {
    USBSerial.println("Error, failed to mount newly formatted filesystem!");
    while(1) delay(1);
  }

  // Done!
  USBSerial.println("Flash chip successfully formatted with new empty filesystem!");


  */




  // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
  usb_msc.setID("Adafruit", "External Flash", "1.0");

  // Set callback
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);

  // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setCapacity(flash.size()/512, 512);

  // MSC is ready for read/write
  usb_msc.setUnitReady(true);
  
  usb_msc.begin();

  // Init file system on the flash
  fatfs.begin(&flash);



  USBSerial.println("Adafruit TinyUSB Mass Storage External Flash example");
  USBSerial.print("JEDEC ID: 0x"); USBSerial.println(flash.getJEDECID(), HEX);
  USBSerial.print("Flash size: "); USBSerial.print(flash.size() / 1024); USBSerial.println(" KB");

  fs_changed = true; // to print contents initially


  //delay (1000);
}

void loop()
{
  /*
  if (fs_changed)
  {
    fs_changed = false;
    
    if ( !root.open("/") )
    {
      USBSerial.println("open root failed");
      //return;
    }

    USBSerial.println("Flash contents:");

    // Open next file in root.
    // Warning, openNext starts at the current directory position
    // so a rewind of the directory may be required.
    while ( file.openNext(&root, O_RDONLY) )
    {
      file.printFileSize(&USBSerial);
      USBSerial.write(' ');
      file.printName(&USBSerial);
      if ( file.isDir() )
      {
        // Indicate a directory.
        USBSerial.write('/');
      }
      USBSerial.println();
      file.close();
    }

    root.close();

    USBSerial.println();
    delay(1000); // refresh every 0.5 second
  }

*/
  digitalWrite(LED_BUILTIN, (millis() % 200) < 100);
  static int t = 0;
  if (millis() - t > 1000)
  {
    t = millis();
    USBSerial.println(millis());
  }
}


// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and 
// return number of copied bytes (must be multiple of block size) 
int32_t msc_read_cb (uint32_t lba, void* buffer, uint32_t bufsize)
{
  // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  return flash.readBlocks(lba, (uint8_t*) buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and 
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb (uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  digitalWrite(LED_BUILTIN, HIGH);

  // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  return flash.writeBlocks(lba, buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb (void)
{
  // sync with flash
  flash.syncBlocks();

  // clear file system's cache to force refresh
  fatfs.cacheClear();

  fs_changed = true;

  digitalWrite(LED_BUILTIN, LOW);
}




//--------------------------------------------------------------------+
// fatfs diskio
//--------------------------------------------------------------------+
extern "C"
{

DSTATUS disk_status ( BYTE pdrv )
{
  (void) pdrv;
	return 0;
}

DSTATUS disk_initialize ( BYTE pdrv )
{
  (void) pdrv;
	return 0;
}

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
  (void) pdrv;
	return flash.readBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
  (void) pdrv;
  return flash.writeBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  (void) pdrv;

  switch ( cmd )
  {
    case CTRL_SYNC:
      flash.syncBlocks();
      return RES_OK;

    case GET_SECTOR_COUNT:
      *((DWORD*) buff) = flash.size()/512;
      return RES_OK;

    case GET_SECTOR_SIZE:
      *((WORD*) buff) = 512;
      return RES_OK;

    case GET_BLOCK_SIZE:
      *((DWORD*) buff) = 8;    // erase block size in units of sector size
      return RES_OK;

    default:
      return RES_PARERR;
  }
}

}
