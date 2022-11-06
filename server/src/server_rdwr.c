#include "server_rdwr.h"
//链表管理结构体全局变量
struct client_list *client_list_store;
//链表读写锁
pthread_rwlock_t client_list_rwlock;

//读写锁死锁保护函数
static void handler(void *arg)
{
    printf("[%u] is cancelled.\n",(unsigned)pthread_self());
    pthread_rwlock_t *pm = (pthread_rwlock_t*)arg;
    pthread_rwlock_unlock(pm);
}

//客户端发送消息线程函数
void *send_task(void *arg)
{
    //将自己设置为分离属性
    pthread_detach(pthread_self());
    //int ret;
    client_t *client_info = (client_t *)arg;
    for(;;)
    {
        if(client_info->send_bit == 1 && client_info->send_buf != NULL)  //有数据要发送
        {
            send(client_info->socket, client_info->send_buf ,INFO_SIZE(client_info->send_buf->size),0);
            free(client_info->send_buf);
            client_info->send_buf = NULL;
            client_info->send_bit = 0;  //数据已发送
        }
    }
}

//客户端消息处理线程函数
//传入客户端信息的链表节点
void *recv_task(void *arg)
{
    //将自己设置为分离属性
    pthread_detach(pthread_self());
    int ret;
    static struct info_rdwr recv_buf = {0};
    //解析地址
    client_t *client_info = (client_t *)arg;
    char *ip = inet_ntoa(client_info->addr.sin_addr);
    int port = ntohs(client_info->addr.sin_port);
//注册登陆部分**************************************************************
    //登陆注册
    ret = login(client_info->socket);
    if(ret <= 0)
    {
        printf("client down\n");    //打印下线客户端的信息
        delete_client_t(client_list_store, client_info); //删除客户端链表节点
        close(client_info->socket); //关闭监听套接字
        return NULL;
    }
//*****************************************************************************
    /*向客户端发送在线列表*/
    for(int i = 0; i <client_list_store->nodeNumber; i++)
    {

    }
    /*监听客户端*/
    client_t *p =NULL;
    for(;;)
    {
        memset(&recv_buf, 0, sizeof(recv_buf));
        ret = recv(client_info->socket,&recv_buf,sizeof(recv_buf),0);
        if(ret <= 0)
        {
            printf("client down\n");    //打印下线客户端的信息
            delete_client_t(client_list_store, client_info); //删除客户端链表节点
            close(client_info->socket); //关闭监听套接字
            return NULL;
        }

        //解析消息
        printf("接收到的信息[ip:%s:%d]func:%d buf:%s ret:%d\n",ip,port, recv_buf.func,recv_buf.buf,ret);
        p = client_list_store->head;
        /*功能码识别*/
        switch(recv_buf.func)
        {
            case 10:    //收到请求转发
                pthread_cleanup_push(handler,(void*)&client_list_rwlock);    //压栈
                pthread_rwlock_rdlock(&client_list_rwlock);  //读锁
                //遍历链表
                for(int i = 0; i <client_list_store->nodeNumber;i++)    //在链表中找到对应地址的链表节点
                {
                    if(recv_buf.other_addr.sin_addr.s_addr == p->addr.sin_addr.s_addr &&
                        recv_buf.other_addr.sin_port == p->addr.sin_port)
                    {
                        while(p->send_bit == 1);    //等待未发送的消息发送完毕
                        recv_buf.other_addr.sin_addr.s_addr = client_info->addr.sin_addr.s_addr;//将消息源地址放入封包中
                        recv_buf.other_addr.sin_port = client_info->addr.sin_port;
                        p->send_buf = malloc(sizeof(struct info_rdwr));
                        memcpy(p->send_buf, &recv_buf, sizeof(struct info_rdwr));   //将要发送的信息拷贝至目标客户端所在节点
                        p->send_bit = 1;    //让相应客户端线程发送消息
                        break;
                    }
                    p = p->next;
                }


            /*应答*/
                client_info->send_buf = malloc(sizeof(struct info_rdwr));
                memset(client_info->send_buf, 0, sizeof(struct info_rdwr));
                if(p == NULL)   //转发失败
                {
                    client_info->send_buf->other_addr = recv_buf.other_addr;    //将目标地址返回给源
                    fprintf(stderr, "[%s:%d]转发失败,查无此客户端\n", inet_ntoa(recv_buf.other_addr.sin_addr),
                            ntohs(recv_buf.other_addr.sin_port));
                }
                else    //转发成功
                {
                    client_info->send_buf->other_addr = p->addr;    //将目标地址返回给源
                    client_info->send_buf->buf[0] = 1;
                }

                client_info->send_buf->func = 11;   //应答功能码
                client_info->send_buf->size = 1;
                client_info->send_bit = 1; //发送
                pthread_rwlock_unlock(&client_list_rwlock);  //解锁
                pthread_cleanup_pop(0);    //弹栈，释放保护函数，但不执行此函数
            break;
        }
    }
}

//接收超时
void rd_timeout(int socket, long sec)
{
    struct timeval timeout={sec,0}; //设置时间
    int ret = setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    if(ret<0)
    {
        perror("setsocketopt error");
        return;
    }
}

//登陆注册
int login(int socket_client)
{
    int ret = 0;
    //消息超时机制
    rd_timeout(socket_client, 3);   //3秒超时
    for(;;)
    {
        struct info_rdwr recv_buf1 = {0};
        memset(&recv_buf1, 0, sizeof(recv_buf1));
        //接收请求
        ret = recv(socket_client,&recv_buf1,sizeof(recv_buf1),0);
        if(ret <= 0  && errno == EAGAIN)
        {
            perror("消息超时");
            close(socket_client);   //此处验证超时直接踢掉
            socket_client = -1;
            return -1;
        }
        //响应请求
        char send_buf[1024] = "OK";
        send(socket_client,send_buf,sizeof(send_buf),0);



        rd_timeout(socket_client, 0xFFFF);   //超时恢复
        printf("登陆成功！\n");
        return socket_client;
    }
}

//初始化服务器
int server_init(void)
{
    int ret;
    //初始化读写锁
    pthread_rwlock_init(&client_list_rwlock,NULL);
    //初始化服务端套接字
    int socket_fd = socket(AF_INET,SOCK_STREAM,0);
    if(socket_fd < 0)
    {
        perror("socket fail");
        return -1;
    }
    //设置端口复用，防止重复绑定本机IP失败(在笔记2里面)
    int optval = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,&optval, sizeof(optval));
    //绑定服务器
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;//地址族
    server_addr.sin_port = htons(SERVER_PORT);//host to net将本机端口转换为网络端口
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);//将本机ip转换为网络ip
    ret = bind(socket_fd,(struct sockaddr *)&server_addr,sizeof(server_addr));
    if(ret < 0)
    {
        perror("bind fail");
        return -1;
    }
    //监听
    ret = listen(socket_fd,20);
    if(ret < 0)
    {
        perror("listen fail");
        return -1;
    }
    return socket_fd;
}

//地址写入
void address_write(struct sockaddr_in *addr, char *ip, int port)
{
    addr->sin_family = AF_INET;//地址族
    addr->sin_port = htons(port);//host to net
    addr->sin_addr.s_addr = inet_addr(ip);//将本机ip转换为网络ip
}

/**************************************链表代码段**************************************/

//创建一条双向链表
struct client_list *create_list(void)
{
//1.申请链表的头结点的内存空间
    struct client_list *listHead=malloc(sizeof(struct client_list));
    if(listHead==NULL)
    {
        printf("malloc listHead error\n");
        return NULL;
    }
//2.初始化头结点的成员
    listHead->head=NULL;
    listHead->tail=NULL;
    listHead->nodeNumber=0;

    return listHead;
}

//函数作用：新建一个结点并初始化，且尾插至链表
//入口参数：
//listHead:链表管理结构体
//socket:客户端套接字
//addr:客户端地址及端口号
//account:客户端账号
//password:客户端密码
client_t  *new_node(struct client_list *listHead, int socket, struct sockaddr_in *addr,
                   char *account, char *password)
{
//1.新建一个结点（申请一个结构体的内存空间）
    client_t *newNode = malloc(sizeof(client_t));
    if(newNode == NULL)
    {
        printf("malloc newNode error\n");
        return NULL;
    }
//2、初始化新建结点
    newNode->socket = socket;
    //地址赋值
    if(addr != NULL)
        newNode->addr = *addr;
    else
        memset(&newNode->addr, 0, sizeof(struct sockaddr_in));
    //账号赋值
    if(account != NULL)
        strcpy(newNode->account, account);
    else
        memset(&newNode->account, 0, sizeof(newNode->account));
    //密码赋值
    if(password != NULL)
        strcpy(newNode->password, password);
    else
        memset(&newNode->password, 0, sizeof(newNode->password));

    newNode->send_bit = 0;
    //memset(&newNode->send_buf, 0, sizeof(newNode->send_buf));
    newNode->send_buf = NULL;
    //节点指针
    newNode->prev = NULL;
    newNode->next = NULL;
    pthread_cleanup_push(handler,(void*)&client_list_rwlock);    //压栈
    pthread_rwlock_wrlock(&client_list_rwlock);  //写锁

//链表尾插
    if(listHead->nodeNumber == 0)   //从无到有
    {
        listHead->head = listHead->tail = newNode;
    }else   //从少到多
    {
        //链表的尾结点的next指向新结点
        listHead->tail->next = newNode;
        //新结点的prev指向尾结点
        newNode->prev = listHead->tail;  //此时tail还是指向上一个节点
        //更新尾结点
        listHead->tail = newNode;
    }
//链表的结点数加1
    listHead->nodeNumber++;

    pthread_rwlock_unlock(&client_list_rwlock);  //解锁
    pthread_cleanup_pop(0);    //弹栈，释放保护函数，但不执行此函数
    return newNode;
}

//遍历客户端信息
void print_all_client(struct client_list *listHead)
{
    int j;
    client_t *p = listHead->head;
    for(j=0;j<listHead->nodeNumber;j++)
    {
        printf("[%d]socket_client:%d ip:%s port:%d\n",j+1,
                p->socket,
               inet_ntoa(p->addr.sin_addr),
               ntohs(p->addr.sin_port));
        p = p->next;
    }

}

//删除单个节点
void delete_client_t(struct client_list *listHead, client_t *del_client)
{
    pthread_cleanup_push(handler,(void*)&client_list_rwlock);    //压栈
    pthread_rwlock_wrlock(&client_list_rwlock);  //写锁
    if(listHead == NULL || del_client == NULL)
    {
        perror("del client_t fail");
        return;
    }
    if(del_client == listHead->head)    //首节点
    {
        //更新首结点
        listHead->head = del_client->next;
        //删除结点的下一个结点的prev指向NULL
        if(del_client->next != NULL)
            del_client->next->prev = NULL;
        //删除结点的next指向NULL
        del_client->next = NULL;
    }
    else if(del_client == listHead->tail)   //尾结点
    {
        //(1)更新尾结点
        listHead->tail = del_client->prev;
        //(2)删除结点断链接
        del_client->prev = NULL;
        //(3)尾结点的next指向NULL
        if(listHead->tail != NULL)
            listHead->tail->next = NULL;
    }else
    {
        //(1)删除结点的下一个结点的prev指向删除结点的上一个结点
        del_client->next->prev = del_client->prev;
        //(2)删除结点的上一个结点的next指向删除结点的下一个结点
        del_client->prev->next = del_client->next;
        //(3)删除结点的链接
        del_client->prev = NULL;
        del_client->next = NULL;
    }
    free(del_client);
    //删除的结点数-1
    listHead->nodeNumber--;
    if(listHead->nodeNumber == 0)
    {
        listHead->tail = listHead->head = NULL;
    }
    pthread_rwlock_unlock(&client_list_rwlock);  //解锁
    pthread_cleanup_pop(0);    //弹栈，释放保护函数，但不执行此函数
}



/**************************************shell界面代码**************************************/

//程序开始打印服务端信息
void start_message_print(void)
{
    system("clear");
    fprintf(stderr,
            "░░░░░░░█▐▓▓░████▄▄▄█▀▄▓▓▓▌█\n"
            "░░░░░▄█▌▀▄▓▓▄▄▄▄▀▀▀▄▓▓▓▓▓▌█\n"
            "░░░▄█▀▀▄▓█▓▓▓▓▓▓▓▓▓▓▓▓▀░▓▌█\n"
            "░░█▀▄▓▓▓███▓▓▓███▓▓▓▄░░▄▓▐█▌\n"
            "░█▌▓▓▓▀▀▓▓▓▓███▓▓▓▓▓▓▓▄▀▓▓▐█\n"
            "▐█▐██▐░▄▓▓▓▓▓▀▄░▀▓▓▓▓▓▓▓▓▓▌█▌\n"
            "█▌███▓▓▓▓▓▓▓▓▐░░▄▓▓███▓▓▓▄▀▐█\n"
            "█▐█▓▀░░▀▓▓▓▓▓▓▓▓▓██████▓▓▓▓▐█\n"
            "▌▓▄▌▀░▀░▐▀█▄▓▓██████████▓▓▓▌█▌\n"
            "▌▓▓▓▄▄▀▀▓▓▓▀▓▓▓▓▓▓▓▓█▓█▓█▓▓▌█\n"
            "█▐▓▓▓▓▓▓▄▄▄▓▓▓▓▓▓█▓█▓█▓█▓▓▓▐█\n"
            "====================================================\n");
    fprintf(stderr,"Welcome to Tcp_telegreat!\n");
    fprintf(stderr,"作者：冯嘉豪\n");
    fprintf(stderr,"Time Now: ");
    system("date"); //显示当前时间
    usleep(100000);
    fprintf(stderr,"Server_IP: %s:%d\n", SERVER_IP, SERVER_PORT);    //显示服务端ip
    fprintf(stderr,"====================================================\n");
}
