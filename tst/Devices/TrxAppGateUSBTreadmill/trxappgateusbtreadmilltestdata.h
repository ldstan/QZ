﻿#pragma once

#include "Devices/bluetoothdevicetestdata.h"
#include "trxappgateusbtreadmill.h"

class TrxAppGateUSBTreadmillTestData : public BluetoothDeviceTestData {

protected:
    void configureExclusions() override;

    bool configureSettings(DeviceDiscoveryInfo& info, bool enable) const override {
        info.toorx_bike = !enable;
        return true;
    }
public:
    TrxAppGateUSBTreadmillTestData();

    deviceType get_expectedDeviceType() const override { return deviceType::TrxAppGateUSBTreadmill; }

    bool get_isExpectedDevice(bluetoothdevice * detectedDevice) const override {
        return dynamic_cast<trxappgateusbtreadmill*>(detectedDevice)!=nullptr;
    }
};

