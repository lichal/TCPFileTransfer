#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int flagsRCV[5];
    printf("Enter a port number:\n ");
    char port[20];
    fgets(port, 20, stdin);
    int portNum = atoi(port);
    
    
    struct sockaddr_in serveraddr;
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(portNum);
    serveraddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec=1;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));

    int len = sizeof(serveraddr);
    
    // Asks for file name
    printf("Enter a file name: ");
    char filename[256];
    
    
    
    fgets(filename, 256, stdin);
    int length = strlen(filename);
    sscanf(filename, "%s", filename);
    
    

    sendto(sockfd, filename, strlen(filename), 0, (struct sockaddr*)&serveraddr,sizeof(serveraddr));
    
    // Make a file with same name request but start with new.
    char newName[256] = "new";
    strcat(newName, filename);
    FILE* file_out = fopen(newName, "wb");
    
    
    int BUFFER_SIZE = 256;
    // buffer temporary holds the pakckets in case of packet lost.
    int buffer[10][BUFFER_SIZE];
    // recvData temporary holds current packet.
    int recvData[BUFFER_SIZE];

    int LFR = -1;
    int LAF = 0;
    int RWS = 5;
    int FLAG;

    while (1) {
        int packetPos = 1;
        LAF = LFR + 6;
        while((LFR + packetPos) < LAF){
            int n = recvfrom(sockfd,recvData,sizeof(int)*BUFFER_SIZE,0,(struct sockaddr*) &serveraddr, &len);
            
            if(n==-1){
                break;
            }
            
            int FLAG = recvData[0];
            printf("FLAG: %d\n", FLAG);
            
            if ((FLAG <= (LFR + RWS) % 10) || (FLAG > LFR && FLAG <= LFR + 5)) {
                int write = 1;
                while(write < 256) {
                    buffer[FLAG][write] = recvData[write];
                    write++;
                }
                
                int SENDFLAG[1];
                SENDFLAG[0] = FLAG;
                flagsRCV[packetPos - 1] = FLAG;
                sendto(sockfd, SENDFLAG, sizeof(int)*1, 0, (struct sockaddr*)&serveraddr,sizeof(serveraddr));
            }
            packetPos++;
        }
        
        int i = 0;
        printf("LFR = %d\nLAF = %d\n",LFR,LAF);
        while (LFR + 1 < LAF) {
            int push = 0;
            int j = 0;
            printf("%d\n", flagsRCV[j]);
            while (j < 5) {
                 int write = 1;
                printf("%d\n", flagsRCV[j]);
                printf("LFR = %d\n",((LFR + 1) % 10));
                if (flagsRCV[j] == ((LFR + 1) % 10)) {
                    int write = 1;
                    while(write < 256){
                        if (buffer[((LFR + 1) % 10)][write] == EOF) {
                            printf("File transfer finish\n");
                            fclose(file_out);
                            close(sockfd);
                            return 0;
                            break;
                        }
                        fputc(buffer[((LFR + 1) % 10)][write], file_out);
                        write++;
                    }
                    if (buffer[((LFR + 1) % 10)][write] == EOF) {
                        break;
                    }
                    LFR++;
                }
                j++;
            }
        }
    }
    fclose(file_out);
    close(sockfd);
}

