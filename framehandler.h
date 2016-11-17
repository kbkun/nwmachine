#ifndef FRAMEHANDLER_H_
#define FRAMEHANDLER_H_

#include <QObject>
#include <QTime>
#include <QSettings>
#include <stdint.h>
#include <string>

#include "kama.pb.h"

const unsigned char F_START = 235;  /*маркер начала кадра*/
const unsigned char F_END = 156;    /*маркер конца кадра*/

namespace Angle {
    enum _Angle {Azimuth, Elevation};
}

class FrameHandler : public QObject
{
	Q_OBJECT    
public:
    explicit    FrameHandler(QObject *parent = 0);
    void 		readFrame(QByteArray* buf);
    bool 		getAngleChannelState();

private:
    QByteArray  	mBuffer;
    QByteArray      measFrame;
	long			frameCounter;
	long			brokenFrame;
    kama::protocol::Envelope msg;
    kama::protocol::Frame frame;
    uchar			decodeDistChTrackMode()   { return measFrame.at(5) >> 5; }
    uchar			decodeAngleChTrackMode()  { return (measFrame.at(5) & 0x1C) >> 2; }
    bool            decodeRespSignMode()      { return measFrame.at(5) & 0x01; }
    bool            decodeGainCtrl()          { return measFrame.at(6) >> 6; }
    bool            decodeFreqCtrl()          { return (measFrame.at(6) & 0x20) >> 5; }
    bool            decodeAntennaPoint()      { return (measFrame.at(6) & 0x10) >> 4; }
    bool            decodeDistIsValid()       { return measFrame.at(6) & 0x01; }
    uchar           decodeAgcLevel()          { return measFrame.at(7); }
    int             decodeTimeMode()          { return (measFrame.at(8) & 0x40) >> 6; }
    std::string     decodeTime();
    double			decodeAngle(Angle::_Angle a);
    int             decodeDist();
    unsigned char	crcKama(unsigned char *b, unsigned int len);
    void            handleServiceFrame();

public slots:
    void receiveChar(unsigned char ch, int workNum, int launchNum, bool mainWork);

private slots:
    void handle(int workNum, int launchNum, bool mainWork);

signals:
    void frameRefreshed(int workNum, int lauchNum, bool mainWork);
    void readyToSend(const kama::protocol::Envelope& msg);
    void broken(bool);
    void startSession();
    void endSession();
};

#endif /* FRAMEHANDLER_H_ */
