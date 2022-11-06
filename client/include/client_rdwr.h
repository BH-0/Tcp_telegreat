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

//账号密码
struct account_password
{
    char account[32];  //账号
    char password[16];  //密码
};

//存储客户端信息的链表节点
typedef struct client_t
{
    struct sockaddr_in addr; //地址
    char account[32];  //账号
    struct client_t *prev;
    struct client_t *next;
}client_t;

//链表管理结构体
struct client_list{
    client_t *head;//保存链表的数据首结点的位置
    client_t *tail;//保存链表的数据尾结点的位置
    int nodeNumber;//保存链表结点的数量
};

//计算封包发送大小
#define INFO_SIZE(BUF_SIZE) (sizeof(struct sockaddr_in)+sizeof(int)+sizeof(int)+BUF_SIZE+1)

//全局变量声明
//链表管理结构体全局变量
extern struct client_list *client_list_store;
//链表读写锁
extern pthread_rwlock_t client_list_rwlock;
//账号密码
extern struct account_password client_account_password;

//接收线程
void *recv_task(void *arg);

//输入登陆注册信息
int scanf_login(void);

//登陆注册
int login(int socket_client);

//地址写入
void address_write(struct sockaddr_in *addr, char *ip, int port);

//创建一条双向链表
struct client_list *create_list(void);

//函数作用：新建一个结点并初始化，且尾插至链表
//入口参数：
//listHead:链表管理结构体
//addr:客户端地址及端口号
//account:客户端账号
client_t  *new_node(struct client_list *listHead, struct sockaddr_in *addr, char *account);

//遍历客户端信息
void print_all_client(struct client_list *listHead);

//删除单个节点
void delete_client_t(struct client_list *listHead, client_t *del_client);

#endif //__CLIENT_RDWR_H
