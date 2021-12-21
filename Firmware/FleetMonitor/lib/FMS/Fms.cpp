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