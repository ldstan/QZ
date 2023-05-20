#ifndef NORDICTRACKIFITADBTREADMILL_H
#define NORDICTRACKIFITADBTREADMILL_H

#include <QtCore/qbytearray.h>

#ifndef Q_OS_ANDROID
#include <QtCore/qcoreapplication.h>
#else
#include <QtGui/qguiapplication.h>
#endif
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qtimer.h>

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QThread>
#include <QUdpSocket>

#include "treadmill.h"
#include "virtualbike.h"
#include "virtualtreadmill.h"

#ifdef Q_OS_IOS
#include "ios/lockscreen.h"
#endif

class nordictrackifitadbtreadmillLogcatAdbThread : public QThread {
    Q_OBJECT

  public:
    explicit nordictrackifitadbtreadmillLogcatAdbThread(QString s);

    void run() override;

  signals:
    void onSpeedInclination(double speed, double inclination);

  private:
    double speed = 0;
    double inclination = 0;
    QString name;
    struct adbfile {
        QDateTime date;
        QString name;
    };

    QString runAdbCommand(QString command);
    void runAdbTailCommand(QString command);
};

class nordictrackifitadbtreadmill : public treadmill {
    Q_OBJECT
  public:
    nordictrackifitadbtreadmill(bool noWriteResistance, bool noHeartService);
    bool connected() override;

    void *VirtualTreadmill();
    void *VirtualDevice() override;
    bool canStartStop() override { return false; }

  private:
    void forceIncline(double incline);
    void forceSpeed(double speed);
    double getDouble(QString v);

    QTimer *refresh;
    virtualtreadmill *virtualTreadmill = nullptr;
    virtualbike *virtualBike = nullptr;

    uint8_t sec1Update = 0;
    QDateTime lastRefreshCharacteristicChanged = QDateTime::currentDateTime();
    uint8_t firstStateChanged = 0;
    uint16_t m_watts = 0;

    bool initDone = false;
    bool initRequest = false;

    bool noWriteResistance = false;
    bool noHeartService = false;

    QUdpSocket *socket = nullptr;
    QHostAddress lastSender;

    nordictrackifitadbtreadmillLogcatAdbThread *logcatAdbThread = nullptr;

#ifdef Q_OS_IOS
    lockscreen *h = 0;
#endif

#ifdef Q_OS_ANDROID
    QString lastCommand = "";
#endif

  signals:
    void disconnected();
    void debug(QString string);

  private slots:

    void onSpeedInclination(double speed, double inclination);

    void processPendingDatagrams();
    void changeInclinationRequested(double grade, double percentage);

    void update();
};

#endif // NORDICTRACKIFITADBTREADMILL_H
