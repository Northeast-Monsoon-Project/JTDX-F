#ifndef JTDXF_NTP_INTERVAL_HPP
#define JTDXF_NTP_INTERVAL_HPP
#include <QList>
inline QList<int> ntpIntervals() { return {1,5,10,15,30,60,180,360,540,720}; }
inline int ntpIntervalMinutes(int value) { return ntpIntervals().contains(value)?value:15; }
#endif
