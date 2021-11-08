// Copyright 2020-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "board_common.h"
#include "board.h"

static const char *TAG = "BOARD_COMMON";
static bool s_board_is_init = false;



ATTR_WEAK esp_err_t iot_board_init(void)
{
    if(s_board_is_init) {
        return ESP_OK;
    }
    //int ret;

    s_board_is_init = true;
    ESP_LOGI(TAG,"Board Info: %s", iot_board_get_info());
    ESP_LOGI(TAG,"Board Init Done ...");
    return ESP_OK;
}

ATTR_WEAK esp_err_t iot_board_deinit(void)
{
    if(!s_board_is_init) {
        return ESP_OK;
    }
    //int ret;


    s_board_is_init = false;
    ESP_LOGI(TAG,"Board Deinit Done ...");
    return ESP_OK;
}

ATTR_WEAK bool iot_board_is_init(void)
{
    return s_board_is_init;
}

ATTR_WEAK board_res_handle_t iot_board_get_handle(int id)
{
    board_res_handle_t handle;
    switch (id)
    {
    default:
        handle = NULL;
        break;
    }
    return handle;
}

ATTR_WEAK char* iot_board_get_info()
{
    static char* info = BOARD_NAME;
    return info;
}
