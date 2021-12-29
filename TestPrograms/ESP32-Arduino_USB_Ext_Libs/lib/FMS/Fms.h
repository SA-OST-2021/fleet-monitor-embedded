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

#pragma once

#include <Arduino.h>
#include "driver/twai.h"

class Fms{
    public:
        Fms();
        Fms(twai_message_t msg);

        uint8_t getPriority() const;
        uint16_t getPgn() const;
        uint8_t getSourceAddress() const;
        const uint8_t* getData() const;

        TickType_t getTimeSinceLastUpdate(){
            return xTaskGetTickCount() - last_update;
        }

        bool compareData(const Fms& b);

        // Operator Overloading
        inline bool operator==(const Fms& rhs){
            return pgn == rhs.getPgn();
        }
        
        inline bool operator!=(const Fms& rhs){
            return pgn != rhs.getPgn();
        }

        Fms& operator=(const Fms& b){
            // Guard self assignment
            if (this == &b)
                return *this;

            priority = b.getPriority();
            pgn = b.getPgn();
            source_address = b.getSourceAddress();
            memcpy(data, b.getData(), 8);
            last_update = xTaskGetTickCount();
            return *this;
        }

    private:
        uint8_t priority;
        uint16_t pgn;
        uint8_t source_address;
        uint8_t data[8];
        TickType_t last_update;
};