#include "serial.h"
#include <iostream>

using namespace std;


int main(int argc, char * argv[])
{
    string dev = "/dev/ttyUSB0";
    Serial sel((char *