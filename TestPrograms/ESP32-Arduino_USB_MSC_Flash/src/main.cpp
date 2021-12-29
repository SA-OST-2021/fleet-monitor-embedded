#include <Arduino.h>
#include "USB.h"

#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"

#include "ff.h"
#include "diskio.h"

#define LED_BUILTIN 5
#define DISK_LABEL  "MONITOR"

int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize);
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
void msc_flush_cb(void);


Adafruit_FlashTransport_ESP32 flashTransport;
Adafruit_SPIFlash flash(&flashTransport);

FatFileSystem fatfs;

FatFile root;
FatFile file;

Adafruit_USBD_MSC usb_msc;

bool forceFormatDisk = false;
bool fs_changed;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  //USB.begin();
  //USB.productName("Fleet-Monitor");
  //Serial.begin(0);
  //Serial.println("ESP32-Arduino_USB_MSC_Flash");

  while(!Serial) delay(10);

  if (!flash.begin()) {
    Serial.println("Error, failed to initialize flash chip!");
    while (1) yield();
  }

  Serial.println("Adafruit TinyUSB Mass Storage External Flash example");
  Serial.print("JEDEC ID: 0x");
  Serial.println(flash.getJEDECID(), HEX);
  Serial.print("Flash size: ");
  Serial.print(flash.size() / 1024);
  Serial.println(" KB");

  usb_msc.setID("Onway AG", "Fleet-Monitor", "1.0");
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);  // Set callback
  usb_msc.setCapacity(flash.size() / 512,
                      512);    // Set disk size, block size should be 512 regardless of spi flash page size
  usb_msc.setUnitReady(true);  // MSC is ready for read/write
  usb_msc.begin();

  if (!fatfs.begin(&flash) || forceFormatDisk)  // Check if disk must be formated
  {
    Serial.println("No FAT File system found, try to format disk...");

    FATFS elmchamFatfs;
    uint8_t workbuf[4096];  // Working buffer for f_fdisk function.

    Serial.println("Partitioning flash with 1 primary partition...");
    DWORD plist[] = {100, 0, 0, 0};      // 1 primary partition with 100% of space.
    uint8_t buf[512] = {0};              // Working buffer for f_fdisk function.
    FRESULT r = f_fdisk(0, plist, buf);  // Partition the flash with 1 partition that takes the entire space.
    if (r != FR_OK) {
      Serial.print("Error, f_fdisk failed with error code: ");
      Serial.println(r, DEC);
      while (1)
        ;
    }
    Serial.println("Partitioned flash!");
    Serial.println("Creating and formatting FAT filesystem (this takes ~60 seconds)...");
    r = f_mkfs("", FM_FAT | FM_SFD, 0, workbuf, sizeof(workbuf));  // Make filesystem.
    if (r != FR_OK) {
      Serial.print("Error, f_mkfs failed with error code: ");
      Serial.println(r, DEC);
      while (1) yield();
    }

    r = f_mount(&elmchamFatfs, "0:", 1);  // mount to set disk label
    if (r != FR_OK) {
      Serial.print("Error, f_mount failed with error code: ");
      Serial.println(r, DEC);
      while (1) yield();
    }
    Serial.println("Setting disk label to: " DISK_LABEL);
    r = f_setlabel(DISK_LABEL);  // Setting label
    if (r != FR_OK) {
      Serial.print("Error, f_setlabel failed with error code: ");
      Serial.println(r, DEC);
      while (1) yield();
    }
    f_unmount("0:");     // unmount
    flash.syncBlocks();  // sync to make sure all data is written to flash
    Serial.println("Formatted flash!");
    if (!fatfs.begin(&flash))  // Check new filesystem
    {
      Serial.println("Error, failed to mount newly formatted filesystem!");
      while (1) delay(1);
    }
    Serial.println("Flash chip successfully formatted with new empty filesystem!");
  }

  

  fs_changed = true;

  while (!Serial) yield();

  // Check if a directory called 'test' exists and create it if not there.
  // Note you should _not_ add a trailing slash (like '/test/') to directory names!
  // You can use the same exists function to check for the existance of a file too.
  if (!fatfs.exists("/test")) {
    Serial.println("Test directory not found, creating...");

    // Use mkdir to create directory (note you should _not_ have a trailing slash).
    fatfs.mkdir("/test");

    if (!fatfs.exists("/test")) {
      Serial.println("Error, failed to create directory!");
      while (1) yield();
    } else {
      Serial.println("Created directory!");
    }
  }

  // You can also create all the parent subdirectories automatically with mkdir.
  // For example to create the hierarchy /test/foo/bar:
  Serial.println("Creating deep folder structure...");
  if (!fatfs.exists("/test/foo/bar")) {
    Serial.println("Creating /test/foo/bar");
    fatfs.mkdir("/test/foo/bar");

    if (!fatfs.exists("/test/foo/bar")) {
      Serial.println("Error, failed to create directory!");
      while (1) yield();
    } else {
      Serial.println("Created directory!");
    }
  }

  // This will create the hierarchy /test/foo/baz, even when /test/foo already exists:
  if (!fatfs.exists("/test/foo/baz")) {
    Serial.println("Creating /test/foo/baz");
    fatfs.mkdir("/test/foo/baz");

    if (!fatfs.exists("/test/foo/baz")) {
      Serial.println("Error, failed to create directory!");
      while (1) yield();
    } else {
      Serial.println("Created directory!");
    }
  }

  // Create a file in the test directory and write data to it.
  // Note the FILE_WRITE parameter which tells the library you intend to
  // write to the file.  This will create the file if it doesn't exist,
  // otherwise it will open the file and start appending new data to the
  // end of it.
  File writeFile = fatfs.open("/test/test.txt", FILE_WRITE);
  if (!writeFile) {
    Serial.println("Error, failed to open test.txt for writing!");
    while (1) yield();
  }
  Serial.println("Opened file /test/test.txt for writing/appending...");

  // Once open for writing you can print to the file as if you're printing
  // to the serial terminal, the same functions are available.
  writeFile.println("Florian Baumgartner");
  writeFile.print("Hello number: ");
  writeFile.println(123456, DEC);
  writeFile.print("Hello hex number: 0x");
  writeFile.println(123456, HEX);

  // Close the file when finished writing.
  writeFile.close();
  Serial.println("Wrote to file /test/test.txt!");

  // Now open the same file but for reading.
  File readFile = fatfs.open("/test/test.txt", FILE_READ);
  if (!readFile) {
    Serial.println("Error, failed to open test.txt for reading!");
    while (1) yield();
  }

  // Read data using the same read, find, readString, etc. functions as when using
  // the serial class.  See SD library File class for more documentation:
  //   https://www.arduino.cc/en/reference/SD
  // Read a line of data:
  String line = readFile.readStringUntil('\n');
  Serial.print("First line of test.txt: ");
  Serial.println(line);

  // You can get the current position, remaining data, and total size of the file:
  Serial.print("Total size of test.txt (bytes): ");
  Serial.println(readFile.size(), DEC);
  Serial.print("Current position in test.txt: ");
  Serial.println(readFile.position(), DEC);
  Serial.print("Available data to read in test.txt: ");
  Serial.println(readFile.available(), DEC);

  // And a few other interesting attributes of a file:
  Serial.print("File name: ");
  Serial.println(readFile.name());
  Serial.print("Is file a directory? ");
  Serial.println(readFile.isDirectory() ? "Yes" : "No");

  // You can seek around inside the file relative to the start of the file.
  // For example to skip back to the start (position 0):
  if (!readFile.seek(0)) {
    Serial.println("Error, failed to seek back to start of file!");
    while (1) yield();
  }

  // And finally to read all the data and print it out a character at a time
  // (stopping when end of file is reached):
  Serial.println("Entire contents of test.txt:");
  while (readFile.available()) {
    char c = readFile.read();
    Serial.print(c);
  }

  // Close the file when finished reading.
  readFile.close();

  // You can open a directory to list all the children (files and directories).
  // Just like the SD library the File type represents either a file or directory.
  File testDir = fatfs.open("/test");
  if (!testDir) {
    Serial.println("Error, failed to open test directory!");
    while (1) yield();
  }
  if (!testDir.isDirectory()) {
    Serial.println("Error, expected test to be a directory!");
    while (1) yield();
  }
  Serial.println("Listing children of directory /test:");
  File child = testDir.openNextFile();
  while (child) {
    char filename[64];
    child.getName(filename, sizeof(filename));

    // Print the file name and mention if it's a directory.
    Serial.print("- ");
    Serial.print(filename);
    if (child.isDirectory()) {
      Serial.print(" (directory)");
    }
    Serial.println();
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
  Serial.println("Deleting /test/foo/test2.txt...");
  if (!fatfs.remove("/test/foo/test2.txt")) {
    Serial.println("Error, couldn't delete test.txt file!");
    while (1) yield();
  }
  Serial.println("Deleted file!");

  // Delete a directory with the rmdir command.  Be careful as
  // this will delete EVERYTHING in the directory at all levels!
  // I.e. this is like running a recursive delete, rm -rf *, in
  // unix filesystems!
  Serial.println("Deleting /test directory and everything inside it...");
  if (!testDir.rmRfStar()) {
    Serial.println("Error, couldn't delete test directory!");
    while (1) yield();
  }
  // Check that test is really deleted.
  if (fatfs.exists("/test")) {
    Serial.println("Error, test directory was not deleted!");
    while (1) yield();
  }
  Serial.println("Test directory was deleted!");

  Serial.println("Finished!");
}

void loop() {
  if (fs_changed) {
    fs_changed = false;
    if (!root.open("/")) {
      Serial.println("open root failed");
      return;
    }
    Serial.println("Flash contents:");
    while (file.openNext(&root, O_RDONLY)) {
      file.printFileSize(&Serial);
      Serial.write(' ');
      file.printName(&Serial);
      if (file.isDir()) {
        Serial.write('/');
      }
      Serial.println();
      file.close();
    }
    root.close();
    Serial.println();
    delay(1000);
  }

  digitalWrite(LED_BUILTIN, (millis() % 200) < 100);
  static int t = 0;
  if (millis() - t > 1000) {
    t = millis();
    Serial.println(millis());
  }
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
  return flash.readBlocks(lba, (uint8_t*)buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  digitalWrite(LED_BUILTIN, HIGH);
  return flash.writeBlocks(lba, buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host). Used to flush any pending
// cache.
void msc_flush_cb(void) {
  flash.syncBlocks();  // sync with flash
  fatfs.cacheClear();  // clear file system's cache to force refresh
  fs_changed = true;
  digitalWrite(LED_BUILTIN, LOW);
}

//--------------------------------------------------------------------+
// fatfs diskio
//--------------------------------------------------------------------+
extern "C" {
DSTATUS disk_status(BYTE pdrv) {
  (void)pdrv;
  return 0;
}

DSTATUS disk_initialize(BYTE pdrv) {
  (void)pdrv;
  return 0;
}

DRESULT disk_read(BYTE pdrv,     // Physical drive nmuber to identify the drive
                  BYTE* buff,    // Data buffer to store read data
                  DWORD sector,  // Start sector in LBA
                  UINT count     // Number of sectors to read
) {
  (void)pdrv;
  return flash.readBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv,         // Physical drive nmuber to identify the drive
                   const BYTE* buff,  // Data to be written
                   DWORD sector,      // Start sector in LBA
                   UINT count         // Number of sectors to write
) {
  (void)pdrv;
  return flash.writeBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv,  // Physical drive nmuber (0..)
                   BYTE cmd,   // Control code
                   void* buff  // Buffer to send/receive control data
) {
  (void)pdrv;

  switch (cmd) {
    case CTRL_SYNC:
      flash.syncBlocks();
      return RES_OK;

    case GET_SECTOR_COUNT:
      *((DWORD*)buff) = flash.size() / 512;
      return RES_OK;

    case GET_SECTOR_SIZE:
      *((WORD*)buff) = 512;
      return RES_OK;

    case GET_BLOCK_SIZE:
      *((DWORD*)buff) = 8;  // erase block size in units of sector size
      return RES_OK;

    default:
      return RES_PARERR;
  }
}
}
