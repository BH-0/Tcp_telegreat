//
// Created by 10031 on 2022/11/4.
//

#ifndef __SERVER_RDWR_H
#define __SERVER_RDWR_H
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

#define SERVER_IP	"192.168.11.67"	//ubuntu的ip
#define SERVER_PORT	60000

//账号密码
struct account_password
{
    char account[32];  //账号
    char password[16];  //密码
};

//存储客户端信息的链表节点
typedef struct client_t
{
    int socket; //存储客户端的套接字
    struct sockaddr_in addr; //地址
    char account[32];  //账号
    volatile int send_bit; //待发标志    0无数据 1有数据   //等数据发出后才装填 //收发都要监听此标志位
    struct info_rdwr *send_buf; //待发数据，用堆空间转存
    struct client_t *prev;
    struct client_t *next;
}client_t;

//链表管理结构体
struct client_list{
    client_t *head;//保存链表的数据首结点的位置
    client_t *tail;//保存链表的数据尾结点的位置
    int nodeNumber;//保存链表结点的数量
};

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

//全局变量声明
//链表管理结构体全局变量
extern struct client_list *client_list_store;
//链表读写锁
extern pthread_rwlock_t client_list_rwlock;

//客户端发送消息线程函数
void *send_task(void *arg);

//客户端消息处理线程函数
void *recv_task(void *arg);

//上下线广播
//入口参数：自己的节点 ，上下线的功能码
void online_broadcast(client_t *client_info, int func);

//接收超时
void rd_timeout(int socket, long sec);

//登陆注册
int login(client_t *client_info);

//初始化服务器
int server_init(void);

//地址写入
void address_write(struct sockaddr_in *addr, char *ip, int port);

//创建一条双向链表
struct client_list *create_list(void);

//函数作用：新建一个结点并初始化，且尾插至链表
//入口参数：
//listHead:链表管理结构体
//socket:客户端套接字
//addr:客户端地址及端口号
//account:客户端账号
//password:客户端密码
client_t  *new_node(struct client_list *listHead, int socket, struct sockaddr_in *addr,
                   char *account, char *password);

//遍历客户端信息
void print_all_client(struct client_list *listHead);

//删除单个节点
void delete_client_t(struct client_list *listHead, client_t *del_client);

//程序开始打印服务端信息
void start_message_print(void);
#endif //__SERVER_RDWR_H
