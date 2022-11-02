﻿#pragma once

#include "Devices/bluetoothdevicetestdata.h"
#include "proformwifitreadmill.h"

class ProFormWiFiTreadmillTestData : public BluetoothDeviceTestData {
protected:
    bool configureSettings(devicediscoveryinfo& info, bool enable) const override {
        info.proformtreadmillip = enable ? this->get_testIP():QString();
        return true;
    }
public:
    ProFormWiFiTreadmillTestData() {
        // any name
        this->addDeviceName("", comparison::StartsWithIgnoreCase);
    }

    deviceType get_expectedDeviceType() const override { return deviceType::ProformWifiTreadmill; }

    bool get_isExpectedDevice(bluetoothdevice * detectedDevice) const override {
        return dynamic_cast<proformwifitreadmill*>(detectedDevice)!=nullptr;
    }
};

