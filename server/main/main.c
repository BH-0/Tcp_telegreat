#include "main.h"
//----------------------------------------------------------------
//此为主程序，所有程序的入口
//----------------------------------------------------------------
int main(int argc, char **argv)
{
    int ret;
    int socket_fd;
    /*初始化客户端链表管理结构体*/
    client_list_store = create_list();
    /*显示服务端信息*/
    start_message_print();

    /*服务端初始化*/
    socket_fd = server_init();
    if(socket_fd < 0)
    {
        perror("server init fail");
        return -1;
    }

/*测试代码*/
#if 0

#endif

/*进入主线程，负责监听客户端上下线，创建子进程*/
#if 1
    while (1)
    {
        //接受客户端 accpet是阻塞函数
        int socket_client;  //客户端socket
        struct sockaddr_in client_addr; //客户端地址获取
        socklen_t addrlen = sizeof(client_addr);
        socket_client = accept(socket_fd,(struct sockaddr *)&client_addr,&addrlen);
        if(socket_client < 0)
        {
            perror("accept fail");
            return -1;
        }
        char *ip = inet_ntoa(client_addr.sin_addr);
        int port = ntohs(client_addr.sin_port);


        //创建节点
        client_t *new_client = new_node(client_list_store, socket_client, &client_addr, NULL, NULL);
        //创建客户端线程
        pthread_t tid_recv, tid_send;
        ret = pthread_create(&tid_recv, NULL, recv_task, new_client);
        if (ret < 0) {
            perror("pthread_create fail");
            return -1;
        }
        ret = pthread_create(&tid_send, NULL, send_task, new_client);
        if (ret < 0) {
            perror("pthread_create fail");
            return -1;
        }


        printf("[新的客户端上线 ip:%s port:%d socket:%d]\n", ip, port, socket_client);

        print_all_client(client_list_store);    //遍历客户端


    }
#endif
    return 0;
}
