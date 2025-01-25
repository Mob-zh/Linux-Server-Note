#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#define MAX_EVENT_NUMBER 1024
#define TCP_BUFFER_SIZE 512
#define UDP_BUFFER_SIZE 1024

int setnonblocking(int fd){
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

void addfd(int epollfd,int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

int main(int argc,char* argv[]){

    if(argc < 3)
    {
        printf("usage:%s ip_address port_number\n",basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int tcp_port = atoi(argv[2]);
    int udp_port = tcp_port+1;
    int ret = 0;

    //创建TCP socket
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(tcp_port);
    inet_pton(AF_INET,ip,&address.sin_addr);

    int tcpsd;
    tcpsd = socket(AF_INET,SOCK_STREAM,0);
    assert(tcpsd >= 0);

    int reuse = 1;
    setsockopt(tcpsd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    ret = bind(tcpsd,(struct sockaddr*)&address,sizeof(address));
    assert(ret >= 0);

    ret = listen(tcpsd,5);
    assert(ret >= 0);

    //创建UDP socket
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(udp_port);
    inet_pton(AF_INET,ip,&address.sin_addr);

    int udpsd;
    udpsd = socket(AF_INET,SOCK_DGRAM,0);
    assert(udpsd >= 0);

    ret = bind(udpsd,(struct sockaddr*)&address,sizeof(address));
    assert(ret != -1);
    

    //监控两个socket
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd >= 0);

    addfd(epollfd,tcpsd);
    addfd(epollfd,udpsd);

    while(1){
        int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if(number < 0){
            printf("epoll fail\n");
            break;
        }
        for(int i = 0; i < number; i++){
            int sockfd = events[i].data.fd;
            if(sockfd == tcpsd){
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(tcpsd,(struct sockaddr*)&client_address,&client_addrlength);
                printf("hello\n");
                char client_ip[20];
                inet_ntop(AF_INET,&client_address.sin_addr,client_ip,20);

                printf("accept:%s %d\n",client_ip,ntohs(client_address.sin_port));
                
                addfd(epollfd,connfd);
            }
            else if(sockfd == udpsd){
                char buf[UDP_BUFFER_SIZE];
                memset(buf,'\0',UDP_BUFFER_SIZE);
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                ret = recvfrom(udpsd,buf,UDP_BUFFER_SIZE-1,0,(struct sockaddr*)&client_address,&client_addrlength);
                char* client_ip;
                inet_ntop(AF_INET,&client_address,client_ip,client_addrlength);
                
                printf("recvfrom:%s %d msg:%s\n",client_ip,ntohs(client_address.sin_port),buf);
                if(ret > 0){
                    sendto(udpsd,buf,ret,0,(struct sockaddr*)&client_address,client_addrlength);
                }
            }
            else if(events[i].events & EPOLLIN){
                char buf[TCP_BUFFER_SIZE];
                while(1){
                    memset(buf,'\0',TCP_BUFFER_SIZE);
                    ret = recv(sockfd,buf,TCP_BUFFER_SIZE-1,0);
                    if(ret < 0){
                        if((errno==EAGAIN)||(errno==EWOULDBLOCK)){
                            break;
                        }
                        printf("tcp client close\n");
                        close(sockfd);
                        break;
                    }
                    else if(ret == 0){
                        printf("tcp client close\n");
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
                        close(sockfd);
                    }
                    else{

                        printf("recv:%s\n",buf);
                        send(sockfd,buf,ret,0);
                    }
                }
            }
            else{
                printf("something else happened\n");
            }
        }

    }
    close(tcpsd);
    return 0;

}


















