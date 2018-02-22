//#define TEST

#ifdef TEST
    crstream<> cresson(Serial);
#else
    #include <SoftwareSerial.h>
    SoftwareSerial            Serial1(3, 4);
    crstream<SoftwareSerial>  cresson(Serial1);
#endif // TEST
