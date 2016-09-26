#include "nmea/nmea.h"
#include <string.h>
#include <stdio.h>

int main()
{
    const char *buff = 
        "$GPGGA,073044.000,2235.8106,N,11359.9026,E,1,8,1.01,143.7,M,-2.4,M,,*41\r\n\
        $GPRMC,070344.000,A,2235.8106,N,11359.9026,E,0.17,220.01,200916,,,A*6A\r\n";

    int it;
    nmeaINFO info;
    nmeaPARSER parser;

    nmea_zero_INFO(&info);
    nmea_parser_init(&parser);

//    for(it = 0; it < 1; ++it)
        nmea_parse(&parser, buff, (int)strlen(buff), &info);

    printf("gps time:\t%04d-%02d-%02d %02d:%02d:%02d\n",
        info.utc.year + 1900,
        info.utc.mon + 1,
        info.utc.day,
        info.utc.hour + 8,
        info.utc.min,
        info.utc.sec);

    printf("gps latitude:\t%f\n", info.lat);
    printf("gps longitude:\t%f\n", info.lon);
    printf("gps altitude:\t%f\n", info.elv);

    nmea_parser_destroy(&parser);

    return 0;
}
