#include "utils.h"
#include "USB.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"
#include "format/ff.h"
#include "format/diskio.h"

extern USBCDC USBSerial;
extern FatFileSystem fatfs;

static Adafruit_USBD_MSC usb_msc;
static Adafruit_FlashTransport_ESP32 flashTransport;
static Adafruit_SPIFlash flash(&flashTransport);
static int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize);
static int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
static void msc_flush_cb(void);
static volatile bool updated = false;

bool utils_init(const char* labelName, bool forceFormat)
{
  USB.begin();
  USB.productName("Fleet-Monitor");
  USBSerial.begin(0);
  USBSerial.println("ESP32-Arduino_USB_MSC_Flash");

  if (!flash.begin())
  {
    USBSerial.println("Error, failed to initialize flash chip!");
    return 0;
  }

  if (!fatfs.begin(&flash) || forceFormat)        // Check if disk must be formated
  {
    USBSerial.println("No FAT File system found, try to format disk...");
    utils_format(labelName);    
  }

  USBSerial.println("Adafruit TinyUSB Mass Storage External Flash example");
  USBSerial.print("JEDEC ID: 0x"); USBSerial.println(flash.getJEDECID(), HEX);
  USBSerial.print("Flash size: "); USBSerial.print(flash.size() / 1024); USBSerial.println(" KB");


  usb_msc.setID("Onway AG", "Fleet-Monitor", "1.0");
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);    // Set callback
  usb_msc.setCapacity(flash.size()/512, 512);     // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setUnitReady(true);                     // MSC is ready for read/write
  usb_msc.begin();


  //while(!USBSerial) yield();
/*

 // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!fatfs.begin(&flash)) {
    USBSerial.println("Error, failed to mount newly formatted filesystem!");
    USBSerial.println("Was the flash chip formatted with the SdFat_format example?");
    while(1) yield();
  }
  USBSerial.println("Mounted filesystem!");

  // Check if a directory called 'test' exists and create it if not there.
  // Note you should _not_ add a trailing slash (like '/test/') to directory names!
  // You can use the same exists function to check for the existance of a file too.
  if (!fatfs.exists("/test")) {
    USBSerial.println("Test directory not found, creating...");
    
    // Use mkdir to create directory (note you should _not_ have a trailing slash).
    fatfs.mkdir("/test");
    
    if ( !fatfs.exists("/test") ) {
      USBSerial.println("Error, failed to create directory!");
      while(1) yield();
    }else {
      USBSerial.println("Created directory!");
    }
  }

  // You can also create all the parent subdirectories automatically with mkdir.
  // For example to create the hierarchy /test/foo/bar:
  USBSerial.println("Creating deep folder structure...");
  if ( !fatfs.exists("/test/foo/bar") ) {
    USBSerial.println("Creating /test/foo/bar");
    fatfs.mkdir("/test/foo/bar");

    if ( !fatfs.exists("/test/foo/bar") ) {
      USBSerial.println("Error, failed to create directory!");
      while(1) yield();
    }else {
      USBSerial.println("Created directory!");
    }
  }

  // This will create the hierarchy /test/foo/baz, even when /test/foo already exists:
  if ( !fatfs.exists("/test/foo/baz") ) {
    USBSerial.println("Creating /test/foo/baz");
    fatfs.mkdir("/test/foo/baz");

    if ( !fatfs.exists("/test/foo/baz") ) {
      USBSerial.println("Error, failed to create directory!");
      while(1) yield();
    }else {
      USBSerial.println("Created directory!");
    }
  }

  // Create a file in the test directory and write data to it.
  // Note the FILE_WRITE parameter which tells the library you intend to
  // write to the file.  This will create the file if it doesn't exist,
  // otherwise it will open the file and start appending new data to the
  // end of it.
  File writeFile = fatfs.open("/test/test.txt", FILE_WRITE);
  if (!writeFile) {
    USBSerial.println("Error, failed to open test.txt for writing!");
    while(1) yield();
  }
  USBSerial.println("Opened file /test/test.txt for writing/appending...");

  // Once open for writing you can print to the file as if you're printing
  // to the serial terminal, the same functions are available.
  writeFile.println("Hello world!");
  writeFile.print("Hello number: "); writeFile.println(123, DEC);
  writeFile.print("Hello hex number: 0x"); writeFile.println(123, HEX);

  // Close the file when finished writing.
  writeFile.close();
  USBSerial.println("Wrote to file /test/test.txt!");

  // Now open the same file but for reading.
  File readFile = fatfs.open("/test/test.txt", FILE_READ);
  if (!readFile) {
    USBSerial.println("Error, failed to open test.txt for reading!");
    while(1) yield();
  }

  // Read data using the same read, find, readString, etc. functions as when using
  // the serial class.  See SD library File class for more documentation:
  //   https://www.arduino.cc/en/reference/SD
  // Read a line of data:
  String line = readFile.readStringUntil('\n');
  USBSerial.print("First line of test.txt: "); USBSerial.println(line);

  // You can get the current position, remaining data, and total size of the file:
  USBSerial.print("Total size of test.txt (bytes): "); USBSerial.println(readFile.size(), DEC);
  USBSerial.print("Current position in test.txt: "); USBSerial.println(readFile.position(), DEC);
  USBSerial.print("Available data to read in test.txt: "); USBSerial.println(readFile.available(), DEC);

  // And a few other interesting attributes of a file:
  USBSerial.print("File name: "); USBSerial.println(readFile.name());
  USBSerial.print("Is file a directory? "); USBSerial.println(readFile.isDirectory() ? "Yes" : "No");

  // You can seek around inside the file relative to the start of the file.
  // For example to skip back to the start (position 0):
  if (!readFile.seek(0)) {
    USBSerial.println("Error, failed to seek back to start of file!");
    while(1) yield();
  }

  // And finally to read all the data and print it out a character at a time
  // (stopping when end of file is reached):
  USBSerial.println("Entire contents of test.txt:");
  while (readFile.available()) {
    char c = readFile.read();
    USBSerial.print(c);
  }

  // Close the file when finished reading.
  readFile.close();

  // You can open a directory to list all the children (files and directories).
  // Just like the SD library the File type represents either a file or directory.
  File testDir = fatfs.open("/test");
  if (!testDir) {
    USBSerial.println("Error, failed to open test directory!");
    while(1) yield();
  }
  if (!testDir.isDirectory()) {
    USBSerial.println("Error, expected test to be a directory!");
    while(1) yield();
  }
  USBSerial.println("Listing children of directory /test:");
  File child = testDir.openNextFile();
  while (child) {
    char filename[64];
    child.getName(filename, sizeof(filename));
    
    // Print the file name and mention if it's a directory.
    USBSerial.print("- "); USBSerial.print(filename);
    if (child.isDirectory()) {
      USBSerial.print(" (directory)");
    }
    USBSerial.println();
    // Keep calling openNextFile to get a new file.
    // When you're done enumerating files an unopened one will
    // be returned (i.e. testing it for true/false like at the
    // top of this while loop will fail).
    child = testDir.openNextFile();
  }

  // If you want to list the files in the directory again call
  // rewindDirectory().  Then openNextFile will start from the
  // top again.
  testDir.rewindDirectory();

  // Delete a file with the remove command.  For example create a test2.txt file
  // inside /test/foo and then delete it.
  File test2File = fatfs.open("/test/foo/test2.txt", FILE_WRITE);
  test2File.close();
  USBSerial.println("Deleting /test/foo/test2.txt...");
  if (!fatfs.remove("/test/foo/test2.txt")) {
    USBSerial.println("Error, couldn't delete test.txt file!");
    while(1) yield();
  }
  USBSerial.println("Deleted file!");

  // Delete a directory with the rmdir command.  Be careful as
  // this will delete EVERYTHING in the directory at all levels!
  // I.e. this is like running a recursive delete, rm -rf *, in
  // unix filesystems!
  USBSerial.println("Deleting /test directory and everything inside it...");
  if (!testDir.rmRfStar()) {
    USBSerial.println("Error, couldn't delete test directory!");
    while(1) yield();
  }
  // Check that test is really deleted.
  if (fatfs.exists("/test")) {
    USBSerial.println("Error, test directory was not deleted!");
    while(1) yield();
  }
  USBSerial.println("Test directory was deleted!");

  USBSerial.println("Finished!");



*/

  return true;
}

bool utils_format(const char* labelName)
{
    FATFS elmchamFatfs;
    uint8_t workbuf[4096];                        // Working buffer for f_fdisk function.
    
    USBSerial.println("Partitioning flash with 1 primary partition...");
    DWORD plist[] = {100, 0, 0, 0};               // 1 primary partition with 100% of space.
    uint8_t buf[512] = {0};                       // Working buffer for f_fdisk function.
    FRESULT r = f_fdisk(0, plist, buf);           // Partition the flash with 1 partition that takes the entire space.
    if (r != FR_OK)
    {
      USBSerial.print("Error, f_fdisk failed with error code: "); USBSerial.println(r, DEC);
      return 0;
    }
    USBSerial.println("Partitioned flash!");
    USBSerial.println("Creating and formatting FAT filesystem (this takes ~60 seconds)...");
    r = f_mkfs("", FM_FAT | FM_SFD, 0, workbuf, sizeof(workbuf));   // Make filesystem.
    if (r != FR_OK)
    {
      USBSerial.print("Error, f_mkfs failed with error code: "); USBSerial.println(r, DEC);
      return 0;
    }
    
    r = f_mount(&elmchamFatfs, "0:", 1);          // mount to set disk label
    if (r != FR_OK)
    {
      USBSerial.print("Error, f_mount failed with error code: "); USBSerial.println(r, DEC);
      return 0;
    }
    USBSerial.print("Setting disk label to: "); USBSerial.println (labelName);
    r = f_setlabel(labelName);                   // Setting label
    if (r != FR_OK)
    {
      USBSerial.print("Error, f_setlabel failed with error code: "); USBSerial.println(r, DEC);
      return 0;
    }
    f_unmount("0:");                              // unmount
    flash.syncBlocks();                           // sync to make sure all data is written to flash
    USBSerial.println("Formatted flash!");
    if (!fatfs.begin(&flash))                     // Check new filesystem
    {
      USBSerial.println("Error, failed to mount newly formatted filesystem!");
      return 0;
    }
    USBSerial.println("Flash chip successfully formatted with new empty filesystem!");
    return 1;
}

bool utils_isUpdated(bool clearFlag)
{
  bool status = updated;
  if(clearFlag)
    updated = false;
  return status;
}


// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and 
// return number of copied bytes (must be multiple of block size) 
static int32_t msc_read_cb (uint32_t lba, void* buffer, uint32_t bufsize)
{
  return flash.readBlocks(lba, (uint8_t*) buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and 
// return number of written bytes (must be multiple of block size)
static int32_t msc_write_cb (uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  return flash.writeBlocks(lba, buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host). Used to flush any pending cache.
static void msc_flush_cb (void)
{
  flash.syncBlocks();       // sync with flash
  fatfs.cacheClear();       // clear file system's cache to force refresh
  updated = true;
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

  DRESULT disk_read
  (
    BYTE pdrv,		      // Physical drive nmuber to identify the drive
    BYTE *buff,		      // Data buffer to store read data
    DWORD sector,	      // Start sector in LBA
    UINT count		      // Number of sectors to read
  )
  {
    (void) pdrv;
    return flash.readBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
  }

  DRESULT disk_write
  (
    BYTE pdrv,			    // Physical drive nmuber to identify the drive
    const BYTE *buff,	  // Data to be written
    DWORD sector,		    // Start sector in LBA
    UINT count			    // Number of sectors to write
  )
  {
    (void) pdrv;
    return flash.writeBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
  }

  DRESULT disk_ioctl
  (
    BYTE pdrv,		      // Physical drive nmuber (0..)
    BYTE cmd,		        // Control code
    void *buff		      // Buffer to send/receive control data
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

