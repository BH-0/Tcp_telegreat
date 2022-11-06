#include "client_rdwr.h"


//接收线程
void *recv_task(void *arg)
{
    //将自己设置为分离属性
    pthread_detach(pthread_self());

    int ret;
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
                printf("[%s:%d]收到信息 func:%d buf:%s ret:%d\n", ip, port, recv_buf.func, recv_buf.buf, ret);
                break;
            case 11: //收到来自服务器的转发应答
                if(recv_buf.buf[0] == 0)    //转发失败
                    printf("[%s:%d]发送失败，客户端不在线\n", ip, port);
                else
                    printf("[%s:%d]发送成功\n", ip, port);
                break;
            case 20:    //在线列表

                break;
            case 21:    //上线

                break;
            case 22:    //下线

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
    for(;;)
    {
        //发送登陆请求
        char send_buf[1024] = "login";
        send(socket_client,send_buf,sizeof(send_buf),0);
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
