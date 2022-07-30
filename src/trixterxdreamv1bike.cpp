#include "trixterxdreamv1bike.h"
#include <cmath>
#include <qmath.h>
#include <QSerialPortInfo>


trixterxdreamv1bike::trixterxdreamv1bike(bool noWriteResistance, bool noHeartService, bool noVirtualDevice, bool noSteering)
{
    // Set the wheel diameter for speed and distance calculations
    this->set_wheelDiameter(DefaultWheelDiameter);

    // QZ things from expected constructor
    this->noWriteResistance = noWriteResistance;
    this->noHeartService = noHeartService;
    this->noVirtualDevice = noVirtualDevice;
    this->noSteering = noSteering;

    if(!bike::connect(&this->port, &trixterxdreamv1serial::request, this, &trixterxdreamv1bike::update))
    {
        throw "Failed to connect to request slot";
    }
}

bool trixterxdreamv1bike::connect(QString portName)
{
    // Get the current time in milliseconds since ancient times.
    // This will be subtracted from further readings from getTime() to get an easier to look at number.
    this->t0 = this->getTime();

    // References to objects for callbacks
    auto thisObject = this;
    auto device=&this->port;

    // tell the client where to get the time
    this->client.set_GetTime([&thisObject]()->uint32_t { return thisObject->getTime();} );

    // tell the client how to send data to the device
    if(!noWriteResistance)
        this->client.set_WriteBytes([device](uint8_t * bytes, int length)->void{ device->write(bytes, length, "");});

    // open the port. This should be at 115200 bits per second.
    this->port.open(portName, 1000);

    // create the timer for the resistance. This only needs to be active when a non-zero resistance is requested.
    this->resistanceTimer = new QTimer(this);
    bike::connect(this->resistanceTimer, &QTimer::timeout, this, &trixterxdreamv1bike::updateResistance);

    // wait for some packets to arrive
    QThread::msleep(500);
    return this->connected();
}

bool trixterxdreamv1bike::testPort(const QString& portName)
{
    try
    {
        trixterxdreamv1bike bike(true, true, true, true); // minimal device
        return bike.connect(portName);
    }
    catch (...)
    {
        return false;
    }

}

QString trixterxdreamv1bike::findPort()
{
    auto availablePorts = trixterxdreamv1serial::availablePorts();

    for(int i=0; i<availablePorts.length(); i++)
    {
        if(testPort(availablePorts[i].portName()))
            return availablePorts[i].portName();
    }

    return nullptr;
}

bool trixterxdreamv1bike::connected()
{
    return (this->getTime()-this->lastPacketProcessedTime) < DisconnectionTimeout;
}


uint32_t trixterxdreamv1bike::getTime()
{
    return static_cast<uint32_t>(QDateTime::currentDateTime().toMSecsSinceEpoch() - this->t0);
}

bool trixterxdreamv1bike::updateClient(const QString& s, trixterxdreamv1client * client)
{
    bool stateChanged = false;

    for(int i=0; i<s.length();i++)
        stateChanged |= client->ReceiveChar(s[i].toLatin1());

    return stateChanged;
}

void trixterxdreamv1bike::update(const QString &s)
{
    // send the bytes to the client and return if there's no change of state
    if(!updateClient(s, &this->client))
        return;

    // Take the most recent state read
    auto state = this->client.getLastState();

    // update the packet count
    this->packetsProcessed++;
    this->lastPacketProcessedTime = this->getTime();

    // update the metrics
    this->LastCrankEventTime = state.LastEventTime;
    if(!this->noHeartService)
        this->Heart.setValue(state.HeartRate);

    // set the speed in km/h
    constexpr double minutesPerHour = 60.0;
    this->Speed.setValue(state.FlywheelRPM * minutesPerHour * this->wheelCircumference);

    // set the distance in km
    this->Distance.setValue(state.CumulativeWheelRevolutions * this->wheelCircumference);

    // set the cadence in revolutions per minute
    this->Cadence.setValue(state.CrankRPM);

    // set the crank revolutions
    this->CrankRevs = state.CumulativeCrankRevolutions;

    // Set the steering
    if(!this->noSteering)
        this->m_steeringAngle.setValue(90.0 / 250.0 * state.Steering -45.0);

}

void trixterxdreamv1bike::changeResistance(int8_t resistanceLevel)
{
    // store the new resistance level. This might be the same as lastRequestedResistance(),Value
    // but it doesn't involve a function call and a cast to get the value.
    this->resistanceLevel = resistanceLevel;

    // don't do anything if resistance is disabled
    if(this->noWriteResistance)
        return;

    if(resistanceLevel==0)
        this->resistanceTimer->stop();
    else
        this->resistanceTimer->start(trixterxdreamv1client::ResistancePulseIntervalMilliseconds);
}

void trixterxdreamv1bike::updateResistance()
{
    this->client.SendResistance(this->resistanceLevel);
}

trixterxdreamv1bike::~trixterxdreamv1bike()
{
    if(this->resistanceTimer)
    {
        this->resistanceTimer->stop();
        delete this->resistanceTimer;
    }

    this->port.quit();


}

void trixterxdreamv1bike::set_wheelDiameter(double value)
{
    // clip the value
    value = std::min(MaxWheelDiameter, std::max(value, MinWheelDiameter));

    // stored as km to avoid dividing by 1000 every time it's used
    this->wheelCircumference = value * M_PI / 1000.0;
}
