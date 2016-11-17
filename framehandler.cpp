#include "framehandler.h"
#include <QDebug>
#include <iostream>
#include <iomanip>

FrameHandler::FrameHandler(QObject *parent)
    : QObject(parent)
    , frameCounter(0)
	, brokenFrame(0)
{
    QObject::connect(this, SIGNAL(frameRefreshed(int, int, bool)), this, SLOT(handle(int, int, bool)));
}


void FrameHandler::receiveChar(unsigned char ch, int workNum, int launchNum, bool mainWork)
{
    static int counter = 0;
    if(mBuffer.endsWith(F_END)) {
        ++counter;
    }
    mBuffer.append(ch);
    if(counter)
        ++counter;
    if(counter == 6) {
        measFrame = mBuffer;
        mBuffer.clear();
        counter = 0;
        emit frameRefreshed(workNum, launchNum, mainWork);
    }
}

void FrameHandler::handle(int workNum, int launchNum, bool mainWork)
{
    /*чистим мусор*/
    int p = measFrame.indexOf(F_START);
    if(p == -1)
        return;
    if(p)
        measFrame = measFrame.remove(0, p);
    frame.Clear();
/*
 * 	проверка контрольной суммы принятого кадра
 */
    unsigned char crc = crcKama((unsigned char*)measFrame.data(), 25);
    if((unsigned char)measFrame.at(25) != crc) {
//        qDebug() << "битый кадр";
        emit broken(true);
        brokenFrame++;
        measFrame.clear();
        return;
    }
/*
 * проверка признака типа информации
 * 		0x0F - кадр с измерительной информацией
 * 		0x03 - кадр без измерительной информации
 */
    if(measFrame.at(3) == 0x03) {
        handleServiceFrame();
        return;
    }
	frameCounter++;
    frame.set_measuring_pointing_number(measFrame.at(2));
    frame.set_information_type(measFrame.at(3));
    frame.set_distance_channel_tracking_mode(decodeDistChTrackMode());
    frame.set_angle_channel_tracking_mode(decodeAngleChTrackMode());
    frame.set_response_signal_mode(decodeRespSignMode());
    frame.set_gain_gontrol(decodeGainCtrl());
    frame.set_frequency_control(decodeFreqCtrl());
    frame.set_antenna_pointing_mode(decodeAntennaPoint());
    frame.set_distance_is_valid(decodeDistIsValid());
    frame.set_agc_level(decodeAgcLevel());
    frame.set_time_mode(decodeTimeMode());
    frame.set_time(decodeTime());
    frame.set_azimuth(decodeAngle(Angle::Azimuth));
    frame.set_elevation(decodeAngle(Angle::Elevation));
    frame.set_distance(decodeDist());
    frame.mutable_frameconext()->set_work_num(workNum);
    frame.mutable_frameconext()->set_launch_num(launchNum);
    frame.mutable_frameconext()->set_main_work(mainWork);
//    qDebug() << "time: " << QString::fromStdString(frame.time());
//    qDebug() << "distance: " << frame.distance();
//    qDebug() << "azimuth: " << frame.azimuth();
//    qDebug() << "elevation" << frame.elevation();
//    qDebug() << "frame num.:" << frameCounter;
    measFrame.clear();
    msg.Clear();
    msg.mutable_frame()->CopyFrom(frame);
    emit readyToSend(msg);
}

//int FrameHandler::decodeTrackingMode(const unsigned char* b)
/*
 * декодирует условия сопровождения в канале углов
 * --------------------------------------------------------------------------------------------------------------
 * | № байта | 				биты					    | 					Информация							|
 * | в кадре | 7	6	5	|	4	3	2	|	1	0	|													 	|
 * --------------------------------------------------------------------------------------------------------------
 * |		 |											|	000 - ручное управление								|
 * |		 |											|	001 - автосопровождение по Аз. у ручное по углам	|
 * |	5	 | 0	0	0	| канал углов	|	0	0	|	010	- автосопровождение по углам					|
 * |		 |											|	100 - управление по целеуказаниям из канала связи	|
 * |		 |											|	101 - управление от внешних устройств				|
 * --------------------------------------------------------------------------------------------------------------
 */
//{
//	int mode = *(b & 0x1C) >> 2; /* 0x1C = 0b00011100 */
//	return mode;
//}

std::string FrameHandler::decodeTime()
/*
 * декодирует значение времени в принятом кадре первчной информации
 * --------------------------------------------------------------------------------------------------------------
 * | № байта | 							биты								    | 	Информация					|
 * | в кадре | 7		6		5		4		3		2		1		0		|							 	|
 * --------------------------------------------------------------------------------------------------------------
 * |   8     | 0		Пр		0		2^16	2^15	2^14	2^13	2^12	| Время проведения измерений 	|
 * |   9     | 0		2^11	2^10	2^9		2^8		2^7		2^6		2^5		| 2^0 = 1 сек				 	|
 * |   10    | 0		2^4		2^3		2^2		2^1		2^0		2^-1	2^-2	| Пр - признак отсчёта времени:	|
 * |		 |																	|	- "1" - московское время	|
 * |		 |																	|	- "0" - стартовое время		|
 * --------------------------------------------------------------------------------------------------------------
 */
{
    QTime time(0, 0, 0);
    uint16_t t, tmp1, tmp2, tmp3, tmp4;
    int ms;
    tmp1 = measFrame.at(8) << 12;
    tmp2 = measFrame.at(9) << 5;
    tmp3 = measFrame.at(10) >> 2;
    tmp4 = measFrame.at(10) & 0x03;
    t = tmp1 + tmp2 + tmp3;
    ms = tmp4 == 0 ? 0 : 250 * tmp4;
    time = time.addSecs(t).addMSecs(ms);

    return time.toString("hh:mm:ss.zzz").toStdString();
}

double FrameHandler::decodeAngle(Angle::_Angle a)
/*
 * декодирует значение угла в принятом кадре первчной информации
 * ------------------------------------------------------------------------------------------------------
 * | № байта | 							биты								    | 	Информация          |
 * | в кадре | 7		6		5		4		3		2		1		0		|                       |
 * -------------------------------------------------------------------------------------------------------
 * |   11    | 0		0		0		0		0		2^14	2^13	2^12	| Азимут                |
 * |   12    | 0		2^11	2^10	2^9		2^8		2^7		2^6		2^5		| 2^0 = 180 град * 2^-14|
 * |   13    | 0		2^4		2^3		2^2		2^1		2^0		0		0		|                       |
 * -------------------------------------------------------------------------------------------------------
 * |   18    | 0		знак	0		0		0		2^14	2^13	2^12	| Угол места            |
 * |   19    | 0		2^11	2^10	2^9		2^8		2^7		2^6		2^5		| 2^0 = 180 град * 2^-14|
 * |   20    | 0		2^4		2^3		2^2		2^1		2^0		0		0		|                       |
 * -------------------------------------------------------------------------------------------------------
 */
{
    int pos;
    if(a == Angle::Azimuth) pos = 11;
    else if(a == Angle::Elevation) pos = 18;
	double angle;
	char sign;
	uint16_t tmp1, tmp2, tmp3;
    sign = (measFrame.at(pos) & 0x40) == 0 ? 1 : -1;
    tmp1 = measFrame.at(pos) << 12;
    tmp2 = measFrame.at(pos + 1) << 5;
    tmp3 = measFrame.at(pos + 2) >> 2;
    angle = sign * (tmp1 + tmp2 + tmp3) * 0.010986328125;	/* 180 * 2^-14 =  0.010986328125 */
    return angle;
}

int FrameHandler::decodeDist()
{
/*
 * декодирует значение дальности в принятом кадре первчной информации
 * ------------------------------------------------------------------------------------------------------
 * | № байта | 							биты								    | 	Информация          |
 * | в кадре | 7		6		5		4		3		2		1		0		|                       |
 * -------------------------------------------------------------------------------------------------------
 * |   14    | 0		0		0		0		0		0   	2^22	2^21	| дальность             |
 * |   15    | 0		2^20	2^19	2^18	2^17	2^16	2^15	2^14	| 2^0 = 1 м             |
 * |   16    | 0		2^13	2^12	2^11	2^10	2^9		2^8		2^7		|                       |
 * |   17    | 0		2^6 	2^5 	2^4 	2^3 	2^2		2^1		2^0		|                       |
 * -------------------------------------------------------------------------------------------------------
 */
    int d;
    uint32_t tmp1, tmp2, tmp3, tmp4;
    tmp1 = measFrame.at(14) << 21;
    tmp2 = measFrame.at(15) << 14;
    tmp3 = measFrame.at(16) << 7;
    tmp4 = measFrame.at(17);
    d = tmp1 + tmp2 + tmp3 + tmp4;
    return d;
}

unsigned char FrameHandler::crcKama(unsigned char *b, unsigned int len)
{
	  unsigned int rg1, i;
	  unsigned char crc = 0;

	  for(i = 0; i < len; i++) {
			rg1 = 0;
			rg1 = crc + b[i];
			if(rg1 >= 0400) ++rg1;
			crc = (unsigned char)rg1;
      }
	  return crc;
}

void FrameHandler::handleServiceFrame()
{
    if((unsigned char)measFrame.at(5) == 65
            && (unsigned char)measFrame.at(6) == 65
            && (unsigned char)measFrame.at(7) == 65) {
        msg.Clear();
        kama::protocol::EndSession end;
        msg.mutable_endsession()->CopyFrom(end);
        emit readyToSend(msg);
        emit endSession();
    } else {
        emit startSession();
    }

}
