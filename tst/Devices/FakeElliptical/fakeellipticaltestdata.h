﻿#pragma once

#include "Devices/bluetoothdevicetestdata.h"
#include "fakeelliptical.h"

class FakeEllipticalTestData : public BluetoothDeviceTestData {

public:
    FakeEllipticalTestData() {
        this->hasSettings = true;
        this->addDeviceName("", comparison::StartsWithIgnoreCase);
    }

    void configureSettings(devicediscoveryinfo& info, bool enable) const override {
        info.fakedevice_elliptical = enable;
    }

    deviceType get_expectedDeviceType() const override { return deviceType::FakeElliptical; }

    bool get_isExpectedDevice(bluetoothdevice * detectedDevice) const override {
        return dynamic_cast<fakeelliptical*>(detectedDevice)!=nullptr;
    }
};

