#include "main.h"

#define SERVER_IP	"192.168.11.67"  //ubuntu的ip
#define SERVER_PORT	60000

int main(int argc, char **argv)
{
#ifdef GEC6816_CMD
    /*硬件初始化*/
    LCD_addr = LCD_init();  //初始化LCD显示
    fd_ts = touch_init("/dev/input/event0");    //初始化触摸屏
    /*字库初始化*/
    Init_Font();
#endif
    int ret;
    //初始化读写锁
    pthread_rwlock_init(&client_list_rwlock,NULL);
    //初始化客户端链表
    client_list_store = create_list();

    //创建套接字
    int socket_fd;
    socket_fd = socket(AF_INET,SOCK_STREAM,0);
    if(socket_fd < 0)
    {
        perror("socket fail");
        return -1;
    }

    //录入登陆信息
    scanf_login();

    //连接服务器
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;//地址族
    server_addr.sin_port = htons(SERVER_PORT);//host to net
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);//将本机ip转换为网络ip
    ret = connect(socket_fd,(struct sockaddr *)&server_addr,sizeof(server_addr));
    if(ret < 0)
    {
        perror("connect fail");
        return -1;
    }
//登陆--------------------------------
    system("clear");
    printf("请求登陆\n");
    //登陆
    ret = login(socket_fd);
    if(ret == 0)
        printf("登陆成功!\n");
//------------------------------------

    /*创建接收线程*/
    pthread_t tid;
    ret = pthread_create(&tid,NULL,recv_task,&socket_fd);
    if(ret < 0)
    {
        perror("pthread_create fail");
        return -1;
    }
    //获取自身端口号和地址
    struct sockaddr_in addr_myself = {0};
    socklen_t len = sizeof(addr_myself);
    getsockname(socket_fd, (struct sockaddr *)&addr_myself, &len);
    printf("================================================================\n");
    printf("我的ip:%s:%d\n", inet_ntoa(addr_myself.sin_addr), ntohs(addr_myself.sin_port));
    printf("================================================================\n");
    printf("tips:输入 lc 可在列表中选择发送消息的对象\n");
    struct info_rdwr send_buf = {0};
    int scanf_c = 0;
    client_t *p = NULL;
    while(1)
    {
        memset(&send_buf, 0, sizeof(send_buf));
        scanf("%s", send_buf.buf);
        if(strcmp(send_buf.buf, "lc") == 0)
        {
            //system("clear");
            while(1) {
                printf("================================================================\n");
                printf("在线用户：\n");
                print_all_client(client_list_store);    //打印列表
                printf("================================================================\n");
                printf("请选择要发送消息的对象序号：");
                scanf_c = 0;
                scanf("%d", &scanf_c);
                if (scanf_c <= 0 || scanf_c > client_list_store->nodeNumber + 0)
                {
                }
                else
                    break;
            }
            p = client_list_store->head;
            for(int i = 1; i < scanf_c; i++)
            {
                p = p->next;    //找到指定的发送对象
            }
            printf("对[%d]ip:%s:%d name:%s 发送消息：\n",scanf_c,
                   inet_ntoa(p->addr.sin_addr),
                   ntohs(p->addr.sin_port),
                   p->account);
        }
        else if(scanf_c != 0 && p != NULL)
        {
            send_buf.func = 10; //转发请求
            send_buf.size = strlen(send_buf.buf);
            send_buf.other_addr = p->addr;  //发送目标
            send(socket_fd,&send_buf,INFO_SIZE(strlen(send_buf.buf)),0);
        }
    }
#if 0 //test
    while (1)
    {


        struct info_rdwr send_buf = {0};
        memset(&send_buf, 0, sizeof(send_buf));

//        send_buf.func = 10; //转发请求
//        sprintf(send_buf.buf,"HELLO");
//        send_buf.size = strlen(send_buf.buf);
//        address_write(&send_buf.other_addr, "192.168.11.3", 53211);   //发送目标地址测试
//
//        //send_buf.other_addr = addr_myself;  //对自己地址发送
//        send(socket_fd,&send_buf,INFO_SIZE(strlen(send_buf.buf)),0);
        printf("请输入\n");
    }
#endif
    return 0;
}
