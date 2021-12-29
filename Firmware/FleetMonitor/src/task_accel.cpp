/*
 * Fleet-Monitor Software
 * Copyright (C) 2021 Institute of Networked Solutions OST
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <USB.h>
#include "LIS2DH12.h"

#define TASK_NETWORKING_FREQ 20

#define ACC_CS   12
#define ACC_MISO 15
#define ACC_MOSI 13
#define ACC_CLK  14

extern USBCDC Serial;

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
  // Initialize the accelerometer
  accel.begin(ACC_CS, ACC_CLK, ACC_MISO, ACC_MOSI);
  // Remove offsets on all axis
  calibrate_accel(accel, &calibration);
  while (1) {
    if (accel.available()) {
      accel_data.ts = xTaskGetTickCount();

      accel_data.acc_x = accel.getX() - calibration.acc_x;
      accel_data.acc_y = accel.getY() - calibration.acc_y;
      accel_data.acc_z = accel.getZ() - calibration.acc_z;

      // Serial.println((String)accel_data.acc_x + (String) " " + (String)accel_data.acc_y + (String) " " +
      //                   accel_data.acc_z);
    }
    vTaskDelayUntil(&task_last_tick, task_freq);
  }
}

/**
 * @brief Average all samples over a period of 5s (100 samples)
 *
 * @param accel is the sensor to read from
 * @param calibration is the offset to be returned
 */
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