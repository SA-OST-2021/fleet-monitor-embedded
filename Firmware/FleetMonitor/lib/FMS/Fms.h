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