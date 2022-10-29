﻿#pragma once

#include "Devices/bluetoothdevicetestdata.h"
#include "rower.h"

class RowerTestData : public BluetoothDeviceTestData {

public:
    RowerTestData() {}

    QStringList get_deviceNames() const override {
        QStringList result;

        return result;
    }

    deviceType get_expectedDeviceType() const override { return deviceType::Rower; }

    bool get_isExpectedDevice(bluetoothdevice * detectedDevice) const override {
        return dynamic_cast<rower*>(detectedDevice)!=nullptr;
    }
};

