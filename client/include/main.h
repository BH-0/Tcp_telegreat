//
// Created by 10031 on 2022/9/29.
//
#ifndef __MAIN_H
#define __MAIN_H
#include <stdio.h>
#include <stdlib.h>
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
#include "client_rdwr.h"

#ifdef GEC6816_CMD
#include "lcd.h"
#include "touch.h"
#include "font.h"
#endif


#endif //__MAIN_H
