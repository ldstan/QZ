#include "bluetoothdevice.h"

#include <QSettings>
#include <QTime>

bluetoothdevice::bluetoothdevice() {}

bluetoothdevice::BLUETOOTH_TYPE bluetoothdevice::deviceType() { return bluetoothdevice::UNKNOWN; }
void bluetoothdevice::start() { requestStart = 1; }
void bluetoothdevice::stop() { requestStop = 1; }
bool bluetoothdevice::supportStateMachine() { return false; }
metric bluetoothdevice::currentHeart() { return Heart; }
metric bluetoothdevice::currentSpeed() { return Speed; }
metric bluetoothdevice::currentInclination() { return Inclination; }
QTime bluetoothdevice::movingTime() {
    int hours = (int)(moving.value() / 3600.0);
    return QTime(hours, (int)(moving.value() - ((double)hours * 3600.0)) / 60.0, ((uint32_t)moving.value()) % 60, 0);
}
QTime bluetoothdevice::elapsedTime() {
    int hours = (int)(elapsed.value() / 3600.0);
    return QTime(hours, (int)(elapsed.value() - ((double)hours * 3600.0)) / 60.0, ((uint32_t)elapsed.value()) % 60, 0);
}
QTime bluetoothdevice::lapElapsedTime() {
    int hours = (int)(elapsed.lapValue() / 3600.0);
    return QTime(hours, (int)(elapsed.lapValue() - ((double)hours * 3600.0)) / 60.0,
                 ((uint32_t)elapsed.lapValue()) % 60, 0);
}

metric bluetoothdevice::currentResistance() { return Resistance; }
metric bluetoothdevice::currentCadence() { return Cadence; }
double bluetoothdevice::currentCrankRevolutions() { return 0; }
uint16_t bluetoothdevice::lastCrankEventTime() { return 0; }
void bluetoothdevice::changeResistance(int8_t resistance) {}
void bluetoothdevice::changePower(int32_t power) {}
void bluetoothdevice::changeInclination(double grade, double percentage) {}

void bluetoothdevice::offsetElapsedTime(int offset) { elapsed += offset; }

QTime bluetoothdevice::currentPace() {
    QSettings settings;
    bool miles = settings.value(QStringLiteral("miles_unit"), false).toBool();
    double unit_conversion = 1.0;
    if (miles) {
        unit_conversion = 0.621371;
    }
    if (Speed.value() == 0) {
        return QTime(0, 0, 0, 0);
    } else {
        double speed = Speed.value() * unit_conversion;
        return QTime(0, (int)(1.0 / (speed / 60.0)),
                     (((double)(1.0 / (speed / 60.0)) - ((double)((int)(1.0 / (speed / 60.0))))) * 60.0), 0);
    }
}

QTime bluetoothdevice::averagePace() {

    QSettings settings;
    bool miles = settings.value(QStringLiteral("miles_unit"), false).toBool();
    double unit_conversion = 1.0;
    if (miles) {
        unit_conversion = 0.621371;
    }
    if (Speed.average() == 0) {
        return QTime(0, 0, 0, 0);
    } else {
        double speed = Speed.average() * unit_conversion;
        return QTime(0, (int)(1.0 / (speed / 60.0)),
                     (((double)(1.0 / (speed / 60.0)) - ((double)((int)(1.0 / (speed / 60.0))))) * 60.0), 0);
    }
}

QTime bluetoothdevice::maxPace() {

    QSettings settings;
    bool miles = settings.value(QStringLiteral("miles_unit"), false).toBool();
    double unit_conversion = 1.0;
    if (miles) {
        unit_conversion = 0.621371;
    }
    if (Speed.max() == 0) {
        return QTime(0, 0, 0, 0);
    } else {
        double speed = Speed.max() * unit_conversion;
        return QTime(0, (int)(1.0 / (speed / 60.0)),
                     (((double)(1.0 / (speed / 60.0)) - ((double)((int)(1.0 / (speed / 60.0))))) * 60.0), 0);
    }
}

double bluetoothdevice::odometer() { return Distance.value(); }
metric bluetoothdevice::calories() { return KCal; }
metric bluetoothdevice::jouls() { return m_jouls; }
uint8_t bluetoothdevice::fanSpeed() { return FanSpeed; };
void *bluetoothdevice::VirtualDevice() { return nullptr; }
bool bluetoothdevice::changeFanSpeed(uint8_t speed) {
    // managing underflow
    if (speed > 230 && FanSpeed < 20) {
        speed = 0;
    } else if (speed < 20 && FanSpeed > 230) {
        // managing overflow
        speed = 100;
    }
    if (speed > 100)
        speed = 100;
    // this is useful when there is a fitmetria fanfit in order to set the current
    // value to the last requested
    FanSpeed = speed;

    requestFanSpeed = speed;
    emit fanSpeedChanged(speed);
    return false;
}
bool bluetoothdevice::connected() { return false; }
metric bluetoothdevice::elevationGain() { return elevationAcc; }
void bluetoothdevice::heartRate(uint8_t heart) { Heart.setValue(heart); }
void bluetoothdevice::disconnectBluetooth() {
    if (m_control) {
        m_control->disconnectFromDevice();
    }
}
metric bluetoothdevice::wattsMetric() { return m_watt; }
void bluetoothdevice::setDifficult(double d) { m_difficult = d; }
double bluetoothdevice::difficult() { return m_difficult; }
void bluetoothdevice::cadenceSensor(uint8_t cadence) { Q_UNUSED(cadence) }
void bluetoothdevice::powerSensor(uint16_t power) { Q_UNUSED(power) }
void bluetoothdevice::speedSensor(double speed) { Q_UNUSED(speed) }

double bluetoothdevice::calculateMETS() { return ((0.048 * m_watt.value()) + 1.19); }

// keiser m3i has a separate management of this, so please check it
void bluetoothdevice::update_metrics(bool watt_calc, const double watts) {

    QDateTime current = QDateTime::currentDateTime();
    double deltaTime = (((double)_lastTimeUpdate.msecsTo(current)) / ((double)1000.0));
    QSettings settings;
    bool power_as_bike = settings.value(QStringLiteral("power_sensor_as_bike"), false).toBool();
    bool power_as_treadmill = settings.value(QStringLiteral("power_sensor_as_treadmill"), false).toBool();

    if (settings.value(QStringLiteral("power_sensor_name"), QStringLiteral("Disabled"))
                .toString()
                .startsWith(QStringLiteral("Disabled")) == false &&
        !power_as_bike && !power_as_treadmill)
        watt_calc = false;

    if (!_firstUpdate && !paused) {
        if (currentSpeed().value() > 0.0 || settings.value(QStringLiteral("continuous_moving"), true).toBool()) {

            elapsed += deltaTime;
        }
        if (currentSpeed().value() > 0.0) {

            moving += deltaTime;
            m_jouls += (m_watt.value() * deltaTime);
            WeightLoss = metric::calculateWeightLoss(KCal.value());
            if (watt_calc) {
                m_watt = watts;
            }
            WattKg = m_watt.value() / settings.value(QStringLiteral("weight"), 75.0).toFloat();
        } else if (m_watt.value() > 0) {

            m_watt = 0;
            WattKg = 0;
        }
    } else if (paused && settings.value(QStringLiteral("instant_power_on_pause"), false).toBool()) {
        // useful for FTP test
        if (watt_calc) {
            m_watt = watts;
        }
        WattKg = m_watt.value() / settings.value(QStringLiteral("weight"), 75.0).toFloat();
    } else if (m_watt.value() > 0) {

        m_watt = 0;
        WattKg = 0;
    }
    METS = calculateMETS();
    if (currentInclination().value() > 0)
        elevationAcc += (currentSpeed().value() / 3600.0) * 1000.0 * (currentInclination().value() / 100.0) * deltaTime;

    _lastTimeUpdate = current;
    _firstUpdate = false;
}

void bluetoothdevice::clearStats() {

    elapsed.clear(true);
    moving.clear(true);
    Speed.clear(false);
    KCal.clear(true);
    Distance.clear(true);
    Heart.clear(false);
    m_jouls.clear(true);
    elevationAcc = 0;
    m_watt.clear(false);
    WeightLoss.clear(false);
    WattKg.clear(false);
    Cadence.clear(false);
}

void bluetoothdevice::setPaused(bool p) {

    paused = p;
    moving.setPaused(p);
    elapsed.setPaused(p);
    Speed.setPaused(p);
    KCal.setPaused(p);
    Distance.setPaused(p);
    Heart.setPaused(p);
    m_jouls.setPaused(p);
    m_watt.setPaused(p);
    WeightLoss.setPaused(p);
    WattKg.setPaused(p);
    Cadence.setPaused(p);
}

void bluetoothdevice::setLap() {

    moving.setLap(true);
    elapsed.setLap(true);
    Speed.setLap(false);
    KCal.setLap(true);
    Distance.setLap(true);
    Heart.setLap(false);
    m_jouls.setLap(true);
    m_watt.setLap(false);
    WeightLoss.setLap(false);
    WattKg.setLap(false);
    Cadence.setLap(false);
}

QStringList bluetoothdevice::metrics() {

    QStringList r;
    r.append(QStringLiteral("Speed"));
    r.append(QStringLiteral("Inclination"));
    r.append(QStringLiteral("Cadence"));
    r.append(QStringLiteral("Elevation"));
    r.append(QStringLiteral("Calories"));
    r.append(QStringLiteral("Odometer"));
    r.append(QStringLiteral("Pace"));
    r.append(QStringLiteral("Resistance"));
    r.append(QStringLiteral("Watt"));
    r.append(QStringLiteral("Weight Loss"));
    r.append(QStringLiteral("AVG Watt"));
    r.append(QStringLiteral("FTP"));
    r.append(QStringLiteral("Heart Rate"));
    r.append(QStringLiteral("Fan"));
    r.append(QStringLiteral("Jouls"));
    r.append(QStringLiteral("Lap Elapsed"));
    r.append(QStringLiteral("Elapsed"));
    r.append(QStringLiteral("Moving Time"));
    r.append(QStringLiteral("Peloton Offset"));
    r.append(QStringLiteral("Peloton Resistance"));
    r.append(QStringLiteral("Date Time"));
    r.append(QStringLiteral("Target Resistance"));
    r.append(QStringLiteral("Target Peloton Resistance"));
    r.append(QStringLiteral("Target Cadence"));
    r.append(QStringLiteral("Target Power"));
    r.append(QStringLiteral("Watt/Kg"));
    return r;
}

uint8_t bluetoothdevice::maxResistance() { return 100; }

uint8_t bluetoothdevice::metrics_override_heartrate() {

    QSettings settings;
    QString setting =
        settings.value(QStringLiteral("peloton_heartrate_metric"), QStringLiteral("Heart Rate")).toString();
    if (!setting.compare(QStringLiteral("Heart Rate"))) {
        return currentHeart().value();
    } else if (!setting.compare(QStringLiteral("Speed"))) {

        return currentSpeed().value();
    } else if (!setting.compare(QStringLiteral("Inclination"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Cadence"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Elevation"))) {

        return elevationGain().value();
    } else if (!setting.compare(QStringLiteral("Calories"))) {

        return calories().value();
    } else if (!setting.compare(QStringLiteral("Odometer"))) {

        return odometer();
    } else if (!setting.compare(QStringLiteral("Pace"))) {

        return currentPace().second();
    } else if (!setting.compare(QStringLiteral("Resistance"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Watt"))) {

        return wattsMetric().value();
    } else if (!setting.compare(QStringLiteral("Weight Loss"))) {

        return weightLoss();
    } else if (!setting.compare(QLatin1String("Watt/Kg"))) {

        return wattKg().value();
    } else if (!setting.compare(QStringLiteral("AVG Watt"))) {

        return wattsMetric().average();
    } else if (!setting.compare(QStringLiteral("FTP"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Fan"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Jouls"))) {

        return jouls().value();
    } else if (!setting.compare(QStringLiteral("Lap Elapsed"))) {

        return lapElapsedTime().second();
    } else if (!setting.compare(QStringLiteral("Elapsed"))) {

        return elapsed.value();
    } else if (!setting.compare(QStringLiteral("Moving Time"))) {

        return movingTime().second();
    } else if (!setting.compare(QStringLiteral("Peloton Offset"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Peloton Resistance"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Date Time"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Target Resistance"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Target Peloton Resistance"))) {

        return 0;
    } else if (!setting.compare(QStringLiteral("Target Power"))) {

        return 0;
    }
    return currentHeart().value();
}

void bluetoothdevice::changeGeoPosition(QGeoCoordinate p, double azimuth) {
    coordinateTS = QDateTime::currentMSecsSinceEpoch();
    coordinate = p;
    this->azimuth = azimuth;
}
QGeoCoordinate bluetoothdevice::currentCordinate() {
    if (coordinateTS) {
        double distance = currentSpeed().value() * ((QDateTime::currentMSecsSinceEpoch() - coordinateTS) / 3600.0);
        QGeoCoordinate c = coordinate.atDistanceAndAzimuth(distance, this->azimuth);
        qDebug() << "currentCordinate" << c << distance << currentSpeed().value();
        return c;
    }
    return coordinate;
}

void bluetoothdevice::workoutEventStateChanged(bluetoothdevice::WORKOUT_EVENT_STATE state) { lastState = state; }
void bluetoothdevice::setInclination(double inclination) { Inclination = inclination; }
