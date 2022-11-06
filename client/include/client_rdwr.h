//
// Created by 10031 on 2022/11/5.
//

#ifndef __CLIENT_RDWR_H
#define __CLIENT_RDWR_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/errno.h>



//收发数据封包格式
struct info_rdwr
{
    struct sockaddr_in other_addr; //其它人的地址
    int func;    //功能码
    int size;       //数据长度
    char buf[1024]; //数据内容
    char NaN;   //占一个字节，作为停止位
};

//计算封包发送大小
#define INFO_SIZE(BUF_SIZE) (sizeof(struct sockaddr_in)+sizeof(int)+sizeof(int)+BUF_SIZE+1)

//接收线程
void *recv_task(void *arg);

//登陆注册
int login(int socket_client);

//地址写入
void address_write(struct sockaddr_in *addr, char *ip, int port);

#endif //__CLIENT_RDWR_H
