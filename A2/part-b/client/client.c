//Group members: Rushil Venkateswar(20CS30045), Jay Kumar Thakur(20CS30024)
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){
    if(argc != 5){
        printf("Incorrect number of arguments\n");
        exit(EXIT_FAILURE);
    }
    int sockfd;
    //Socket creation
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0){
        printf("Socket creation failed\n");
        exit(EXIT_FAILURE);
    }
    //Client receives loadbalancer's ip and port through command line arguments
    struct sockaddr_in lbaddr, recvaddr;
    int addrlen = sizeof(lbaddr);
    lbaddr.sin_family = AF_INET;
    lbaddr.sin_port = htons(atoi(argv[2]));
    lbaddr.sin_addr.s_addr = inet_addr(argv[1]);
    //Client also receives the range [L,R] in which the sleep times should lie
    int L = atoi(argv[3]);
    int R = atoi(argv[4]);
    while(1){
        sleep(2);
        //Generate sleep time values send to loadbalancer
        int sendval = 0;
        if(L == R){
            sendval = L;
        }
        else{
            sendval = rand()%(R - L + 1) + L;
        }
        sendto(sockfd, &sendval, sizeof(sendval), 0, (const struct sockaddr *)&lbaddr, sizeof(lbaddr));
        printf("Sent sleep time: %d\n", sendval);

    }

}