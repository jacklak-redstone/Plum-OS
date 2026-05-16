#pragma once
#include "std/types.hpp"

namespace NET {
    class Net_Device {
    public:
        virtual ~Net_Device() = default;

        virtual bool Init() = 0;

        virtual bool send(const uint8_t *data, uint16_t len) = 0;

        virtual const uint8_t* get_mac() const = 0;

        virtual const char* get_name() const = 0;
    };
}
