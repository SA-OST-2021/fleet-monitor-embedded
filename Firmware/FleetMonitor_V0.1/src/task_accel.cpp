// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <USB.h>
#include "LIS2DH12.h"

#define TASK_NETWORKING_FREQ 20

extern USBCDC USBSerial;

typedef struct {
  TickType_t ts;
  int16_t acc_x, acc_y, acc_z;
} accel_data_t;

void calibrate_accel(LIS2DH12& accel, accel_data_t* calibration);

void task_accel(void* pvParameter) {
  const TickType_t task_freq = TASK_NETWORKING_FREQ;
  TickType_t task_last_tick = xTaskGetTickCount();
  LIS2DH12 accel;
  accel_data_t calibration;
  accel_data_t accel_data;
  accel.begin(12, 14, 15, 13);
  calibrate_accel(accel, &calibration);
  while (1) {
    if (accel.available()) {
      accel_data.ts = xTaskGetTickCount();

      accel_data.acc_x = accel.getX() - calibration.acc_x;
      accel_data.acc_y = accel.getY() - calibration.acc_y;
      accel_data.acc_z = accel.getZ() - calibration.acc_z;

      USBSerial.println((String)accel_data.acc_x + (String) " " + (String)accel_data.acc_y + (String) " " +
                        accel_data.acc_z);
    }
    vTaskDelayUntil(&task_last_tick, task_freq);
  }
}

void calibrate_accel(LIS2DH12& accel, accel_data_t* calibration) {
  int32_t x = 0;
  int32_t y = 0;
  int32_t z = 0;
  for (int i = 0; i < 100; i++) {
    if (accel.available()) {
      x += accel.getX();
      y += accel.getY();
      z += accel.getZ();
    }
    vTaskDelay(20);
  }
  calibration->acc_x = x / 100;
  calibration->acc_y = y / 100;
  calibration->acc_z = z / 100;
}