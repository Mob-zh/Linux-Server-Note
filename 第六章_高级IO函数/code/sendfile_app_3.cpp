#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
intmain(intargc, char *argv[])
{
    if (argc <= 3)
    {
        printf("usage:%sip_addressport_number filename\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    constchar *file_name = argv[3];
    intfilefd = open(file_name, O_RDONLY);
    assert(filefd > 0);
    structstatstat_buf;
    fstat(filefd, &stat_buf);
    structsockaddr_inaddress;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    intsock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    intret = bind(sock, (structsockaddr *)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(sock, 5);
    assert(ret != -1);
    structsockaddr_inclient;
    socklen_tclient_addrlength = sizeof(client);
    intconnfd = accept(sock, (structsockaddr *)&client, &client_addrlength);
    if (connfd < 0)
    {
        printf("errnois:%d\n", errno);
    }
    else
    {
        sendfile(connfd, filefd, NULL, stat_buf.st_size);
        close(connfd);
    }
    close(sock);
    return0;
}