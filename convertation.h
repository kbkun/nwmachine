#ifndef CONVERTATION_H
#define CONVERTATION_H

#include <cmath>

typedef struct _DecToDeg {
    qint16 deg;
    qint16 min;
    qint16 sec;
} DecToDeg;

DecToDeg decToDeg(double d)
{
    int sign = d > 0 ? 1 : -1;
    DecToDeg res;
    res.deg = d;
    res.min = abs((d - res.deg) * 60);
    res.sec = abs(round(((d - res.deg) * 60.0 - sign * res.min) * 60));
    if(res.sec == 60) {
        res.sec = 0;
        ++res.min;
    }

    return res;
}

#endif // CONVERTATION_H
