#include "gpx.h"
#include "math.h"
#include "qdebugfixup.h"
#include <QDomDocument>
#include <QSettings>
#include <QXmlStreamWriter>

gpx::gpx(QObject *parent) : QObject(parent) {}

QList<gpx_altitude_point_for_treadmill> gpx::open(const QString &gpx) {
    QSettings settings;
    bool treadmill_force_speed =
        settings.value(QZSettings::treadmill_force_speed, QZSettings::default_treadmill_force_speed).toBool();
    QFile input(gpx);
    input.open(QIODevice::ReadOnly);
    QDomDocument doc;
    doc.setContent(&input);
    QDomNodeList metadata = doc.elementsByTagName(QStringLiteral("metadata"));
    if (metadata.size()) {
        QDomNodeList list = metadata.at(0).childNodes();
        for (int i = 0; i < list.count(); i++) {
            if (list.at(i).nodeName().toLower() == "video") {
                QString video = list.at(i).toElement().firstChild().nodeValue();
                if (!video.isEmpty()) {
                    videoUrl = video;
                    qDebug() << "gpx::videoUrl " << videoUrl;
                    break;
                }
            }
        }
    }

    QDomNodeList points = doc.elementsByTagName(QStringLiteral("trkpt"));
    for (int i = 0; i < points.size(); i++) {
        QDomNode point = points.item(i);
        QDomNamedNodeMap att = point.attributes();
        QString lat = att.namedItem(QStringLiteral("lat")).nodeValue();
        QString lon = att.namedItem(QStringLiteral("lon")).nodeValue();
        QDomElement ele = point.firstChildElement(QStringLiteral("ele"));
        QDomElement time = point.firstChildElement(QStringLiteral("time"));
        gpx_point g;
        // 2020-10-10T10:54:45
        g.time = QDateTime::fromString(time.text(), Qt::ISODate);
        g.p.setAltitude(ele.text().toDouble());
        g.p.setLatitude(lat.toDouble());
        g.p.setLongitude(lon.toDouble());
        this->points.append(g);
    }

    QList<gpx_altitude_point_for_treadmill> inclinationList;

    if (this->points.isEmpty()) {
        return inclinationList;
    }

    gpx_point pP = this->points.constFirst();

    if (treadmill_force_speed) {
        for (int32_t i = 1; i < this->points.count(); i++) {
            qint64 dT = qAbs(pP.time.secsTo(this->points.at(i).time));

            double distance = this->points.at(i).p.distanceTo(pP.p);
            double elevation = this->points.at(i).p.altitude() - pP.p.altitude();

            if (distance == 0) {
                continue;
            }

            gpx_altitude_point_for_treadmill g;
            g.latitude = pP.p.latitude();
            g.longitude = pP.p.longitude();
            g.elevation = pP.p.altitude();
            
            pP = this->points[i];
            
            g.seconds = this->points.constFirst().time.secsTo(pP.time);
            g.distance = distance / 1000.0;
            g.speed = (distance / 1000.0) * (3600 / dT);
            g.inclination = (elevation / distance) * 100;            
            inclinationList.append(g);
        }
    }
    if (inclinationList.empty()) {
        gpx_point pP = this->points.constFirst();
        double totDistance = 0;
        if (!isnan(this->points.constFirst().p.latitude()) && !isnan(this->points.constFirst().p.longitude()) &&
            QGeoCoordinate(this->points.first().p.latitude(), this->points.first().p.longitude())
                    .distanceTo(QGeoCoordinate(this->points.constLast().p.latitude(),
                                               this->points.constLast().p.longitude())) < 300) {
            // to create the circuit
            this->points.append(this->points.constFirst());
            this->points.last().time = this->points.at(this->points.count() - 2).time;
        }

        for (int32_t i = 1; i < this->points.count(); i++) {
            double distance = this->points.at(i).p.distanceTo(pP.p);
            double elevation = this->points.at(i).p.altitude() - pP.p.altitude();

            if (distance == 0) {
                continue;
            }

            gpx_altitude_point_for_treadmill g;
            g.latitude = pP.p.latitude();
            g.longitude = pP.p.longitude();
            g.elevation = pP.p.altitude();

            pP = this->points[i];
            
            g.distance = distance / 1000.0;
            totDistance += g.distance;
            g.inclination = (elevation / distance) * 100;            
            g.seconds = this->points.constFirst().time.secsTo(pP.time);
            /*qDebug() << qSetRealNumberPrecision(10) << i << g.distance << g.inclination << g.elevation << g.latitude
             << g.longitude << totDistance << pP.time;*/
            inclinationList.append(g);
        }
    }

    return inclinationList;
}

void gpx::save(const QString &filename, QList<SessionLine> session, bluetoothdevice::BLUETOOTH_TYPE type) {
    if (session.isEmpty()) {
        return;
    }

    QFile output(filename);
    output.open(QIODevice::WriteOnly);
    QXmlStreamWriter stream(&output);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    // header
    stream.writeStartElement(QStringLiteral("gpx"));
    stream.writeAttribute(QStringLiteral("creator"), QStringLiteral("qdomyos-zwift"));
    stream.writeAttribute(QStringLiteral("xmlns:xsi"), QStringLiteral("http://www.w3.org/2001/XMLSchema-instance"));
    stream.writeAttribute(
        QStringLiteral("xsi:schemaLocation"),
        QStringLiteral(
            "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd "
            "http://www8.garmin.com/xmlschemas/GpxExtensions/v3 http://www8.garmin.com/xmlschemas/GpxExtensionsv3.xsd "
            "http://www8.garmin.com/xmlschemas/TrackPointExtension/v1 "
            "http://www8.garmin.com/xmlschemas/TrackPointExtensionv1.xsd "
            "http://www8.garmin.com/xmlschemas/PowerExtension/v1 "
            "http://www8.garmin.com/xmlschemas/PowerExtensionv1.xsd"));
    stream.writeAttribute(QStringLiteral("version"), QStringLiteral("1.1"));
    stream.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.topografix.com/GPX/1/1"));
    stream.writeAttribute(QStringLiteral("xmlns:gpxtpx"),
                          QStringLiteral("http://www8.garmin.com/xmlschemas/TrackPointExtension/v1"));
    stream.writeAttribute(QStringLiteral("xmlns:gpxx"),
                          QStringLiteral("http://www8.garmin.com/xmlschemas/GpxExtensions/v3"));
    stream.writeAttribute(QStringLiteral("xmlns:gpxpx"),
                          QStringLiteral("http://www8.garmin.com/xmlschemas/PowerExtension/v1"));
    stream.writeAttribute(QStringLiteral("xmlns:gpxdata"), QStringLiteral("http://www.cluetrust.com/XML/GPXDATA/1/0"));

    stream.writeStartElement(QStringLiteral("metadata"));
    stream.writeTextElement(QStringLiteral("time"),
                            session.at(0).time.toString(QStringLiteral("yyyy-MM-ddTHH:mm:ssZ")));
    stream.writeEndElement();

    stream.writeStartElement(QStringLiteral("trk"));
    stream.writeTextElement(QStringLiteral("name"), session.at(0).time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));

    if (type == bluetoothdevice::TREADMILL || type == bluetoothdevice::ELLIPTICAL) {
        stream.writeTextElement(QStringLiteral("type"), QStringLiteral("0"));
    } else {
        stream.writeTextElement(QStringLiteral("type"), QStringLiteral("53"));
    }

    stream.writeStartElement(QStringLiteral("trkseg"));
    for (const SessionLine &s : qAsConst(session)) {
        if (s.speed > 0) {
            stream.writeStartElement(QStringLiteral("trkpt"));
            stream.writeAttribute(QStringLiteral("lat"), QStringLiteral("0"));
            stream.writeAttribute(QStringLiteral("lon"), QStringLiteral("0"));
            stream.writeTextElement(QStringLiteral("ele"),
                                    QStringLiteral("0")); // replace with the cumulative inclination
            stream.writeTextElement(QStringLiteral("time"), s.time.toString(QStringLiteral("yyyy-MM-ddTHH:mm:ssZ")));
            stream.writeTextElement(QStringLiteral("speed"), QString::number(s.speed / 3.6)); // meter per second
            stream.writeStartElement(QStringLiteral("extensions"));
            stream.writeTextElement(QStringLiteral("power"), QString::number(s.watt));
            stream.writeTextElement(QStringLiteral("gpxdata:hr"), QString::number(s.heart));
            stream.writeTextElement(QStringLiteral("gpxdata:cadence"), QString::number(s.cadence));
            stream.writeStartElement(QStringLiteral("gpxtpx:TrackPointExtension"));
            stream.writeTextElement(QStringLiteral("gpxtpx:speed"), QString::number(s.speed / 3.6)); // meter per second
            stream.writeTextElement(QStringLiteral("gpxtpx:hr"), QString::number(s.heart));
            stream.writeTextElement(QStringLiteral("gpxtpx:cad"), QString::number(s.cadence));
            stream.writeTextElement(QStringLiteral("gpxtpx:distance"), QString::number(s.distance));
            stream.writeEndElement(); // gpxtpx:TrackPointExtension
            stream.writeStartElement(QStringLiteral("gpxpx:PowerExtension"));
            stream.writeTextElement(QStringLiteral("gpxpx:PowerInWatts"), QString::number(s.watt));
            stream.writeEndElement(); // gpxtpx:PowerExtension
            stream.writeEndElement(); // extensions
            stream.writeEndElement(); // trkpt
        }
    }
    stream.writeEndElement(); // trkseg
    stream.writeEndElement(); // trk
    stream.writeEndElement(); // gpx

    stream.writeEndDocument();
}
