//Group members: Rushil Venkateswar(20CS30045), Jay Kumar Thakur(20CS30024)
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Incorrect number of arguments\n");
        exit(EXIT_FAILURE);
    }

    int sockfd;
    //Socket creation
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0){
        printf("Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    //Client receives server IP and port through command line arguments
    struct sockaddr_in servaddr;
    int addrlen = sizeof(servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    
    //Client sends data to server periodically
    while(1){
        sleep(5);
        int randv = rand()%128;
        sendto(sockfd, &randv, sizeof(randv), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        printf("Data sent: %d\n", randv);
        fflush(stdout);
    }

}
