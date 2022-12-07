#include "characteristicwriteprocessore005.h"
#include "elliptical.h"
#include "ftmsbike.h"
#include "treadmill.h"
#include "wahookickrsnapbike.h"
#include <QSettings>
#include <QtMath>

CharacteristicWriteProcessorE005::CharacteristicWriteProcessorE005(double bikeResistanceGain,
                                                                   uint8_t bikeResistanceOffset, bluetoothdevice *bike,
                                                                   // CharacteristicNotifier2AD9 *notifier,
                                                                   QObject *parent)
    : CharacteristicWriteProcessor(bikeResistanceGain, bikeResistanceOffset, bike, parent) {}

int CharacteristicWriteProcessorE005::writeProcess(quint16 uuid, const QByteArray &data, QByteArray &reply) {
    if (data.size()) {
        bluetoothdevice::BLUETOOTH_TYPE dt = Bike->deviceType();
        if (dt == bluetoothdevice::BIKE) {
            QSettings settings;
            bool force_resistance =
                settings.value(QZSettings::virtualbike_forceresistance, QZSettings::default_virtualbike_forceresistance)
                    .toBool();
            bool erg_mode = settings.value(QZSettings::zwift_erg, QZSettings::default_zwift_erg).toBool();
            char cmd = data.at(0);
            emit ftmsCharacteristicChanged(QLowEnergyCharacteristic(), data);
            if (cmd == wahookickrsnapbike::_setSimMode && data.count() >= 7) {
                weight = ((double)((uint16_t)data.at(1)) + (((uint16_t)data.at(2)) >> 8)) / 100.0;
                rrc = ((double)((uint16_t)data.at(3)) + (((uint16_t)data.at(4)) >> 8)) / 1000.0;
                wrc = ((double)((uint16_t)data.at(5)) + (((uint16_t)data.at(6)) >> 8)) / 1000.0;
                qDebug() << "weigth" << weight << "rrc" << rrc << "wrc" << wrc;
            } else if (cmd == wahookickrsnapbike::_setSimGrade && data.count() >= 3) {
                uint16_t grade;
                double fgrade;
                grade = (uint16_t)((uint8_t)data.at(1)) + (((uint16_t)((uint8_t)data.at(2))) << 8);
                fgrade = (((((double)grade) / 65535.0) * 2) - 1.0) * 100.0;
                qDebug() << "grade" << grade << "fgrade" << fgrade;
                changeSlope(fgrade * 100.0, rrc, wrc);
            }
        } else if (dt == bluetoothdevice::TREADMILL || dt == bluetoothdevice::ELLIPTICAL) {
        }
        reply.append((quint8)FTMS_RESPONSE_CODE);
        reply.append((quint8)data.at(0));
        reply.append((quint8)FTMS_SUCCESS);
        /*if (notifier) {
            notifier->answer = reply;
        }*/
        return CP_OK;
    } else
        return CP_INVALID;
}
