#include "nordictrackifitadbtreadmilltestdata.h" 
#include "nordictrackifitadbtreadmill.h"

bool NordicTrackIFitADBTreadmillTestData::configureSettings(DeviceDiscoveryInfo &info, bool enable) const {
    if(enable)
        info.nordictrack_2950_ip = this->get_testIP();
    else
        info.nordictrack_2950_ip = QString();

    return true;
}

NordicTrackIFitADBTreadmillTestData::NordicTrackIFitADBTreadmillTestData() : TreadmillTestData("Nordictrack IFit ADB Treadmill") {
    // Allow any name because it's not a bluetooth device
    this->addDeviceName("", comparison::StartsWithIgnoreCase);
}

deviceType NordicTrackIFitADBTreadmillTestData::get_expectedDeviceType() const { return deviceType::NordicTrackIFitADBTreadmill; }

bool NordicTrackIFitADBTreadmillTestData::get_isExpectedDevice(bluetoothdevice *detectedDevice) const {
    return dynamic_cast<nordictrackifitadbtreadmill*>(detectedDevice)!=nullptr;
}
