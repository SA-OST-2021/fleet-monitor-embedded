#include <Arduino.h>
#include "USB.h"

#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"

#define LED_BUILTIN 5


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


// Set to true when PC write to flash
bool fs_changed;


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


  Serial.begin(115200);
  delay(100);
  Serial.println("Test Serial");
  USBSerial.println("Test Florian");


  flash.begin();

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
  USBSerial.print("JEDEC ID: 0x"); Serial.println(flash.getJEDECID(), HEX);
  USBSerial.print("Flash size: "); Serial.print(flash.size() / 1024); Serial.println(" KB");

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