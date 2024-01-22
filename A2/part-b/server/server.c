//Group members: Rushil Venkateswar(20CS30045), Jay Kumar Thakur(20CS30024)
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h> 

int PORT = 8080;
int free_threads[5];
pthread_mutex_t thread_mutex;
typedef struct _func_args{
    int index;
    int sleeptime;
}func_args;
int sockfd = -1;
struct sockaddr_in lbaddr;
long long sleep_tot = 0LL;
//Thread function which sleeps for sleep time passed in arguments
void *sleepThread(void *args){
    func_args *my_args = (func_args *)args;
    int idx = my_args->index;
    int sleept = my_args->sleeptime;
    pthread_mutex_lock(&thread_mutex);
    free_threads[idx] = 1;
    printf("Thread %d sleeping for %d seconds\n", idx, sleept);
    pthread_mutex_unlock(&thread_mutex);
    sleep(sleept);
    
    pthread_mutex_lock(&thread_mutex);
    sleep_tot += sleept;
    free_threads[idx] = 0;
    int sendv = 0;
    //Send the load balancer a message telling a thread has got free 
    sendto(sockfd, &sendv, sizeof(sendv), 0, (const struct sockaddr *)&lbaddr, sizeof(lbaddr));
    int cnt = 0;
    for(int i = 0; i < 5; i++){
        if(!free_threads[i])
            cnt++;
    }
    printf("Thread %d is free now server has %d free threads\nServer has slept for %lld seconds", idx, cnt, sleep_tot);
    pthread_mutex_unlock(&thread_mutex);
}
int main(){
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0){
        printf("Socket creation failed\n");
        exit(EXIT_FAILURE);
    }
    //Socket creation and binding
    struct sockaddr_in servaddr;
    int addrlen = sizeof(servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port  = htons(PORT);
    int lbaddrlen = sizeof(lbaddr);
    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        printf("Bind failed\n");
        exit(EXIT_FAILURE);
    }

   int sleept;
    if(pthread_mutex_init(&thread_mutex, NULL) != 0){
        printf("Mutex lock initializtion failed\n");
        exit(EXIT_FAILURE);
    }
    while(1){
        //Receive messages from load balancer
        recvfrom(sockfd, &sleept, sizeof(sleept), 0, (struct sockaddr *)&lbaddr, &lbaddrlen);
        int sleeping_thread = -1;
        //Check if a thread is free, mark the free thread as not free and invoke the sleepThread function for it
        pthread_mutex_lock(&thread_mutex);
        for(int i = 0; i < 5; i++){
            if(!free_threads[i]){
                sleeping_thread = i;
                break;
            }  
        }
        if(sleeping_thread != -1){
            free_threads[sleeping_thread] = 1;
        }
        pthread_mutex_unlock(&thread_mutex);
        if(sleeping_thread == -1){
            printf("Server can't sleep for %d sec\n", sleept);
        }
        else{
        func_args args;
        args.index = sleeping_thread;
        args.sleeptime = sleept;
        pthread_t  sleep_thread;
        pthread_create(&sleep_thread, NULL, &sleepThread, (void *)&args);
        }
    }
    close(sockfd);
    return 0;
}