
//
// Created by michael on 17-12-15.
//

#include "serial.h"

/**
 * @brief   构造函数，初始化文件句柄
 * @param   dev 类型 char*    串口号
 */
Serial::Serial(char *dev)
{
    fd = open( dev, O_RDWR | O_NOCTTY );         //| O_NOCTTY | O_NDELAY
    if (-1 == fd)
    {
        perror("Can't Open Serial Port");
        exit(0);
    }
}

Serial::~Serial()
{
    close(fd);
}

/**
 * @brief   延迟
 * @param   sec     类型 int  延迟秒数
 * @return  void
 */
void Serial::delay(int sec)
{
    time_t start_time, cur_time;
    time(&start_time);
    do{
        time(&cur_time);
    }while((cur_time - start_time) < sec);
}

/**
 * @brief   设置串口通信速率,数据位，停止位和效验位