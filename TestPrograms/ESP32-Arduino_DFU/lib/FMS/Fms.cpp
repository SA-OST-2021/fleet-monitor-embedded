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

#include "Fms.h"

Fms::Fms(){
    last_update = xTaskGetTickCount();
}

Fms::Fms(twai_message_t msg){
    priority = (msg.identifier & 0x1C000000) >> 26;
    pgn = (msg.identifier & 0x00FFFF00) >> 8;
    source_address = (msg.identifier & 0xFF);
    memcpy(data, msg.data, 8);
    last_update = xTaskGetTickCount();
}

// Returns true if data is the same
bool Fms::compareData(const Fms& b){
    if(pgn != b.getPgn()) return false;
    for(int i = 0; i < 8; i++){
        if(data[i] != b.getData()[i]) return false;
    }
    return true;
}

uint8_t Fms::getPriority() const{
    return priority;
}

uint16_t Fms::getPgn() const{
    return pgn;
}

uint8_t Fms::getSourceAddress() const{
    return source_address;
}

const uint8_t* Fms::getData() const{
    return data;
}