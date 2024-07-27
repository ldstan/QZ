
#include "devices/bike.h"
#include "qdebugfixup.h"
#include <QSettings>

bike::bike() { elapsed.setType(metric::METRIC_ELAPSED); }

virtualbike *bike::VirtualBike() { return dynamic_cast<virtualbike*>(this->VirtualDevice()); }

void bike::changeResistance(resistance_t resistance) {
    QSettings settings;
    double zwift_erg_resistance_up =
        settings.value(QZSettings::zwift_erg_resistance_up, QZSettings::default_zwift_erg_resistance_up).toDouble();
    double zwift_erg_resistance_down =
        settings.value(QZSettings::zwift_erg_resistance_down, QZSettings::default_zwift_erg_resistance_down).toDouble();

    lastRawRequestedResistanceValue = resistance;
    if (autoResistanceEnable) {
        double v = (resistance * m_difficult) + gears();
        if ((double)v > zwift_erg_resistance_up) {
            qDebug() << "zwift_erg_resistance_up filter enabled!";
            v = (resistance_t)zwift_erg_resistance_up;
        } else if ((double)v < zwift_erg_resistance_down) {
            qDebug() << "zwift_erg_resistance_down filter enabled!";
            v = (resistance_t)zwift_erg_resistance_down;
        }
        requestResistance = v;
        emit resistanceChanged(requestResistance);
    }
    RequestedResistance = resistance * m_difficult + gears();
}

void bike::changeInclination(double grade, double percentage) {
    qDebug() << QStringLiteral("bike::changeInclination") << autoResistanceEnable << grade << percentage;
    lastRawRequestedInclinationValue = grade;
    if (autoResistanceEnable) {        
        qDebug() << QStringLiteral("setting bike::requestInclination")  << grade;
        requestInclination = grade;
    }
    emit inclinationChanged(grade, percentage);
}

// originally made for renphobike, but i guess it could be very generic
uint16_t bike::powerFromResistanceRequest(resistance_t requestResistance) {
    // this bike has resistance level to N.m so the formula is Power (kW) = Torque (N.m) x Speed (RPM) / 9.5488
    double cadence = RequestedCadence.value();
    if (cadence <= 0)
        cadence = Cadence.value();
    return (requestResistance * cadence) / 9.5488;
}

void bike::changeRequestedPelotonResistance(int8_t resistance) { RequestedPelotonResistance = resistance; }
void bike::changeCadence(int16_t cadence) { RequestedCadence = cadence; }

void bike::changePower(int32_t power) {

    RequestedPower = power; // in order to paint in any case the request power on the charts

    if (!autoResistanceEnable) {
        qDebug() << QStringLiteral("changePower ignored because auto resistance is disabled");
        return;
    }

    requestPower = power; // used by some bikes that have ERG mode builtin

    if(this->ergModeSupported)
    {
        qDebug() << QStringLiteral("changePower to ") << power << QStringLiteral("W using built-in ERG mode.");
        return;
    }

    QSettings settings;
    bool force_resistance =
        settings.value(QZSettings::virtualbike_forceresistance, QZSettings::default_virtualbike_forceresistance)
            .toBool();

    if(!force_resistance) {
        qDebug() << QStringLiteral("changePower to ") << power << QStringLiteral("W ignored because force_resistance setting is off");
        return;
    }

    double erg_filter_upper =
        settings.value(QZSettings::zwift_erg_filter, QZSettings::default_zwift_erg_filter).toDouble();
    double erg_filter_lower =
        settings.value(QZSettings::zwift_erg_filter_down, QZSettings::default_zwift_erg_filter_down).toDouble();
    double watts = this->wattsMetric().value();
    double deltaDown = watts - ((double)power);
    double deltaUp = -deltaDown;
    qDebug() << QStringLiteral("filter  ") + QString::number(deltaUp) + " " + QString::number(deltaDown) + " " +
                    QString::number(erg_filter_upper) + " " + QString::number(erg_filter_lower);
    if (deltaUp > erg_filter_upper || deltaDown > erg_filter_lower) {
        resistance_t r = (resistance_t)resistanceFromPowerRequest(power);

        qDebug() << QStringLiteral("Changing resistance to ") << r;
        changeResistance(r); // resistance start from 1
    }
}

double bike::gears() { return m_gears; }
void bike::setGears(double gears) {
    QSettings settings;
    qDebug() << "setGears" << gears;
    m_gears = gears;
    settings.setValue(QZSettings::gears_current_value, m_gears);
    if (lastRawRequestedResistanceValue != -1) {
        changeResistance(lastRawRequestedResistanceValue);
    }
}

double bike::currentCrankRevolutions() { return CrankRevs; }
uint16_t bike::lastCrankEventTime() { return LastCrankEventTime; }
metric bike::lastRequestedResistance() { return RequestedResistance; }
metric bike::lastRequestedPelotonResistance() { return RequestedPelotonResistance; }
metric bike::lastRequestedCadence() { return RequestedCadence; }
metric bike::lastRequestedPower() { return RequestedPower; }
metric bike::currentResistance() { return Resistance; }
uint8_t bike::fanSpeed() { return FanSpeed; }
bool bike::connected() { return false; }
uint16_t bike::watts() { return 0; }
metric bike::pelotonResistance() { return m_pelotonResistance; }
resistance_t bike::pelotonToBikeResistance(int pelotonResistance) { return pelotonResistance; }
resistance_t bike::resistanceFromPowerRequest(uint16_t power) { return power / 10; } // in order to have something
void bike::cadenceSensor(uint8_t cadence) { Cadence.setValue(cadence); }
void bike::powerSensor(uint16_t power) { m_watt.setValue(power, false); }

bluetoothdevice::BLUETOOTH_TYPE bike::deviceType() { return bluetoothdevice::BIKE; }

void bike::clearStats() {

    moving.clear(true);
    elapsed.clear(true);
    Speed.clear(false);
    KCal.clear(true);
    Distance.clear(true);
    Distance1s.clear(true);
    Heart.clear(false);
    m_jouls.clear(true);
    elevationAcc = 0;
    m_watt.clear(false);
    WeightLoss.clear(false);

    RequestedPelotonResistance.clear(false);
    RequestedResistance.clear(false);
    RequestedCadence.clear(false);
    RequestedPower.clear(false);
    m_pelotonResistance.clear(false);
    Cadence.clear(false);
    Resistance.clear(false);
    WattKg.clear(false);
    for(int i=0; i<maxHeartZone(); i++) {
        hrZonesSeconds[i].clear(false);
    }    
}

void bike::setPaused(bool p) {

    paused = p;
    moving.setPaused(p);
    elapsed.setPaused(p);
    Speed.setPaused(p);
    KCal.setPaused(p);
    Distance.setPaused(p);
    Distance1s.setPaused(p);
    Heart.setPaused(p);
    m_jouls.setPaused(p);
    m_watt.setPaused(p);
    WeightLoss.setPaused(p);
    m_pelotonResistance.setPaused(p);
    Cadence.setPaused(p);
    Resistance.setPaused(p);
    RequestedPelotonResistance.setPaused(p);
    RequestedResistance.setPaused(p);
    RequestedCadence.setPaused(p);
    RequestedPower.setPaused(p);
    WattKg.setPaused(p);
    for(int i=0; i<maxHeartZone(); i++) {
        hrZonesSeconds[i].setPaused(p);
    }    
}

void bike::setLap() {

    moving.setLap(true);
    elapsed.setLap(true);
    Speed.setLap(false);
    KCal.setLap(true);
    Distance.setLap(true);
    Distance1s.setLap(true);
    Heart.setLap(false);
    m_jouls.setLap(true);
    m_watt.setLap(false);
    WeightLoss.setLap(false);
    WattKg.setLap(false);

    RequestedPelotonResistance.setLap(false);
    RequestedResistance.setLap(false);
    RequestedCadence.setLap(false);
    RequestedPower.setLap(false);
    m_pelotonResistance.setLap(false);
    Cadence.setLap(false);
    Resistance.setLap(false);
    for(int i=0; i<maxHeartZone(); i++) {
        hrZonesSeconds[i].setLap(false);
    }    
}

uint8_t bike::metrics_override_heartrate() {

    QSettings settings;
    QString setting =
        settings.value(QZSettings::peloton_heartrate_metric, QZSettings::default_peloton_heartrate_metric).toString();
    if (!setting.compare(QStringLiteral("Heart Rate"))) {
        return qRound(currentHeart().value());
    } else if (!setting.compare(QStringLiteral("Speed"))) {

        return qRound(currentSpeed().value());
    } else if (!setting.compare(QStringLiteral("Inclination"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Cadence"))) {

        return qRound(Cadence.value());
    } else if (!setting.compare(QStringLiteral("Elevation"))) {

        return qRound(elevationGain().value());
    } else if (!setting.compare(QStringLiteral("Calories"))) {

        return qRound(calories().value());
    } else if (!setting.compare(QStringLiteral("Odometer"))) {

        return qRound(odometer());
    } else if (!setting.compare(QStringLiteral("Pace"))) {

        return currentPace().second();
    } else if (!setting.compare(QStringLiteral("Resistance"))) {

        return qRound(Resistance.value());
    } else if (!setting.compare(QStringLiteral("Watt"))) {

        return qRound(wattsMetric().value());
    } else if (!setting.compare(QStringLiteral("Weight Loss"))) {

        return qRound(weightLoss());
    } else if (!setting.compare(QStringLiteral("AVG Watt"))) {

        return qRound(wattsMetric().average());
    } else if (!setting.compare(QStringLiteral("FTP"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Fan"))) {

        return FanSpeed;
    } else if (!setting.compare(QStringLiteral("Jouls"))) {

        return qRound(jouls().value());
    } else if (!setting.compare(QStringLiteral("Lap Elapsed"))) {

        return lapElapsedTime().second();
    } else if (!setting.compare(QStringLiteral("Elapsed"))) {

        return qRound(elapsed.value());
    } else if (!setting.compare(QStringLiteral("Moving Time"))) {

        return movingTime().second();
    } else if (!setting.compare(QStringLiteral("Peloton Offset"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Peloton Resistance"))) {

        return qRound(pelotonResistance().value());
    } else if (!setting.compare(QStringLiteral("Date Time"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Target Resistance"))) {

        return qRound(RequestedResistance.value());
    } else if (!setting.compare(QStringLiteral("Target Peloton Resistance"))) {

        return qRound(RequestedPelotonResistance.value());
    } else if (!setting.compare(QStringLiteral("Target Power"))) {

        return qRound(RequestedPower.value());
    } else if (!setting.compare(QStringLiteral("Watt/Kg"))) {
        return qRound(wattKg().value());
    } else if (!setting.compare(QStringLiteral("Target Cadence"))) {
        return qRound(RequestedCadence.value());
    }
    return qRound(currentHeart().value());
}

bool bike::inclinationAvailableByHardware() { return false; }

uint16_t bike::wattFromHR(bool useSpeedAndCadence) {
    QSettings settings;
    double watt = 0;
    if (currentCadence().value() == 0 && useSpeedAndCadence == true) {
        return 0;
    }
    if (Heart.value() > 0) {
        int avgP = ((settings.value(QZSettings::power_hr_pwr1, QZSettings::default_power_hr_pwr1).toDouble() *
                     settings.value(QZSettings::power_hr_hr2, QZSettings::default_power_hr_hr2).toDouble()) -
                    (settings.value(QZSettings::power_hr_pwr2, QZSettings::default_power_hr_pwr2).toDouble() *
                     settings.value(QZSettings::power_hr_hr1, QZSettings::default_power_hr_hr1).toDouble())) /
                       (settings.value(QZSettings::power_hr_hr2, QZSettings::default_power_hr_hr2).toDouble() -
                        settings.value(QZSettings::power_hr_hr1, QZSettings::default_power_hr_hr1).toDouble()) +
                   (Heart.value() *
                    ((settings.value(QZSettings::power_hr_pwr1, QZSettings::default_power_hr_pwr1).toDouble() -
                      settings.value(QZSettings::power_hr_pwr2, QZSettings::default_power_hr_pwr2).toDouble()) /
                     (settings.value(QZSettings::power_hr_hr1, QZSettings::default_power_hr_hr1).toDouble() -
                      settings.value(QZSettings::power_hr_hr2, QZSettings::default_power_hr_hr2).toDouble())));
        if (Speed.value() > 0 || useSpeedAndCadence == false) {
            if (avgP < 50) {
                avgP = 50;
            }
            watt = avgP;
        } else {
            watt = 0;
        }
    } else {
        watt = currentCadence().value() * 1.2; // random value cloned from Zwift when HR is not available
    }
    return watt;
}
