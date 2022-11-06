#include "main.h"
//----------------------------------------------------------------
//此为主程序，所有程序的入口
//----------------------------------------------------------------

#define SERVER_IP	"192.168.11.3"  //ubuntu的ip
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
    printf("hello\n");
    int ret;
    //创建套接字
    int socket_fd;
    socket_fd = socket(AF_INET,SOCK_STREAM,0);
    if(socket_fd < 0)
    {
        perror("socket fail");
        return -1;
    }

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
    printf("我的ip:%s:%d\n", inet_ntoa(addr_myself.sin_addr), ntohs(addr_myself.sin_port));


#if 1 //进入主线程
    while (1)
    {
        struct info_rdwr send_buf = {0};
        memset(&send_buf, 0, sizeof(send_buf));

        send_buf.func = 10; //转发请求
        sprintf(send_buf.buf,"HELLO");
        send_buf.size = strlen(send_buf.buf);
        address_write(&send_buf.other_addr, "192.168.11.3", 53211);   //发送目标地址测试

        //send_buf.other_addr = addr_myself;  //对自己地址发送
        send(socket_fd,&send_buf,INFO_SIZE(strlen(send_buf.buf)),0);
        sleep(1);
    }
#endif
    return 0;
}
