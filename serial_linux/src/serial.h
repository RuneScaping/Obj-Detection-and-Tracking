#ifndef SERIAL_H
#define SERIAL_H
#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/signal.h>
#include  <sys/stat.h>
#include  <fcntl.h>
#include  <termios.h>
#include  <errno.h>
#include  <limits.h>
#include  <string>
using namespace std;

class Serial
{
public:
    Serial(char *dev);
    ~Serial();
    void delay(int sec);
    bool setPara(int speed = 9600, int databits = 8, int stopbits = 1, int parity = 'N');
    bool setBaudRate(int speed);
    int writeData(const char *data, int datalength);
    int readData(char *data, int da