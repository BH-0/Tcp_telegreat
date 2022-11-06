#include "client_rdwr.h"
//链表管理结构体全局变量
struct client_list *client_list_store;
//链表读写锁
pthread_rwlock_t client_list_rwlock;
//账号密码
struct account_password client_account_password;

//读写锁死锁保护函数
static void handler(void *arg)
{
    //printf("[%lu] is cancelled.\n",pthread_self());
    pthread_rwlock_t *pm = (pthread_rwlock_t*)arg;
    pthread_rwlock_unlock(pm);
}

//接收线程
void *recv_task(void *arg)
{
    //将自己设置为分离属性
    pthread_detach(pthread_self());
    //static int cnt = 0;
    int ret;
    client_t *p = NULL;
    int socket_client = *(int *)arg;

    struct info_rdwr recv_buf = {0};
    for(;;)
    {
        memset(&recv_buf, 0, sizeof(recv_buf));
        ret = recv(socket_client,&recv_buf,sizeof(recv_buf),0);
        if(ret <= 0)
        {
            printf("client down\n");
            return NULL;
        }

        //解析消息
        char *ip = inet_ntoa(recv_buf.other_addr.sin_addr);
        int port = ntohs(recv_buf.other_addr.sin_port);
        switch(recv_buf.func)   //功能码识别
        {
            case 10:    //接收转发的消息
                printf("[%s:%d]收到信息 func:%d ret:%d buf:%s \n", ip, port, recv_buf.func, ret, recv_buf.buf);
                break;
            case 11: //收到来自服务器的转发应答
                if(recv_buf.buf[0] == 0)    //转发失败
                    printf("[%s:%d]发送失败，客户端不在线\n", ip, port);
                else
                    printf("[%s:%d]发送成功\n", ip, port);
                break;
            case 20:    //在线列表
                if(recv_buf.size == 0) //到达尾帧
                {
//                    printf("在线用户：\n");
//                    print_all_client(client_list_store);
                }
                else {
                    //printf("在线：[%s:%d] name: %s\n", ip, port, recv_buf.buf);
                    new_node(client_list_store, &(recv_buf.other_addr), recv_buf.buf);
                }
                break;
            case 21:    //上线
                printf("上线：[%s:%d] name: %s\n", ip, port, recv_buf.buf);
                new_node(client_list_store, &(recv_buf.other_addr), recv_buf.buf);
                break;
            case 22:    //下线
                p = client_list_store->head;
                //printf("下线--：[%s:%d] name: %s\n", ip, port, recv_buf.buf);
                for(int i = 0; i <client_list_store->nodeNumber; i++)   //遍历链表，寻找那个要删除的节点
                {
                    if(recv_buf.other_addr.sin_addr.s_addr == p->addr.sin_addr.s_addr &&
                       recv_buf.other_addr.sin_port == p->addr.sin_port)
                    {
                        client_t *dp = p;
                        p = p->next;
                        delete_client_t(client_list_store, dp);
                        printf("下线：[%s:%d] name: %s\n", ip, port, recv_buf.buf);
                        continue;
                    }
                    p = p->next;
                }
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

//输入登陆注册信息
int scanf_login(void)
{
    //int ret = 0;
    struct account_password login_buf = {0};
    while (1)
    {
        int n = 0;
        printf("======注册请输入 1 ======\n======登录请输入 2 ======\n");
        scanf("%d", &n);
        if (n != 1 && n != 2)
        {
            printf("输入有误 请重来\n");
            // continue;
        }
        switch(n)
        {
            case 1: //注册
                // 注册
                printf("======注册======\n");
                printf("请输入用户名：");
                scanf("%31s", client_account_password.account);
                printf("\n请输入密码：");
                scanf("%15s", client_account_password.password);

                printf("注册完成，请登录\n");
                break;
            case 2: //登陆
                while (1) {
                    memset(&login_buf, 0, sizeof(login_buf));
                    printf("======登陆======\n");
                    printf("用户名：");
                    scanf("%31s", login_buf.account);
                    printf("\n密码：");
                    scanf("%15s", login_buf.password);

                    if ((strcmp(client_account_password.account, login_buf.account) == 0) &&
                        (strcmp(client_account_password.password, login_buf.password) == 0)) {
                        printf("\n~登录成功~\n");
                        return 0; // 发送给服务器后退出
                    } else {
                        printf("\n!!!密码或用户名错误,请重试!!!\n");
                    }
                }
                break;
        }

    }
}

//登陆注册
int login(int socket_client)
{
    struct info_rdwr send_buf = {0};
    for(;;)
    {
        //发送登陆请求
        send_buf.func = 1;
        send_buf.size = sizeof(struct account_password);
        memcpy(send_buf.buf, &client_account_password,sizeof(struct account_password));
        send(socket_client,&send_buf,sizeof(send_buf),0);
        //等待服务器响应
        char recv_buf[1024] = {0};
        //接收请求
        int ret = recv(socket_client,recv_buf,sizeof(recv_buf),0);
        if(ret <= 0)
        {
            perror("失败");
            return -1;
        }
        //登陆成功跳出循环
        return 0;
    }
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
//addr:客户端地址及端口号
//account:客户端账号
client_t  *new_node(struct client_list *listHead, struct sockaddr_in *addr, char *account)
{
//1.新建一个结点（申请一个结构体的内存空间）
    client_t *newNode = malloc(sizeof(client_t));
    if(newNode == NULL)
    {
        printf("malloc newNode error\n");
        return NULL;
    }
//2、初始化新建结点
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
        printf("[%d]ip:%s:%d name:%s\n",j+1,
               inet_ntoa(p->addr.sin_addr),
               ntohs(p->addr.sin_port),
               p->account);
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

