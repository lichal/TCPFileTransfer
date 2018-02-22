#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    printf("Enter a port number: ");
    char port[20];
    fgets(port, 20, stdin);
    int portNum = atoi(port);
    
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    struct sockaddr_in serveraddr, clientaddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(portNum);
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    
    bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
    int BUFFER_SIZE = 256;
    int FRAME_SIZE = 10;
    
    while (1) {
        int len = sizeof(clientaddr);
        unsigned char filename[1024] = "";
        int n = recvfrom(sockfd, filename, 1024, 0,
                         (struct sockaddr*) &clientaddr, &len);
        if (n == -1) {
            printf("Timed out while waiting to receive.\n");
            continue;
        }
        
        /* Open file and read it in binary. */
        printf("File name received! Opening File: %s\n", filename);
        FILE *fileptr;
        fileptr = fopen(filename, "rb");
        
        /* Checks if the file exist or not. */
        if (!fileptr) {
            char * c = "file does not extist";
            printf("%s %s\n", filename, c);
//            sendto(sockfd, c, strlen(c) + 1, 0, (struct sockaddr*) &clientaddr,
//                   sizeof(clientaddr));
            continue;
        }
        
        /* Determine the file length. */
        long filelen;
        printf("File opened\n");             // Open the file in binary mode
        fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
        filelen = ftell(fileptr); // Get the current byte offset in the file
        rewind(fileptr);           // Jump back to the beginning of the file
        
        /* Loop to send file */
        while (1) {
            
            int LAR = -1;
//            int LFS = 0;
//            int CPS = 0;
            int SWS = 5;

            
            int frames[FRAME_SIZE][BUFFER_SIZE];
            
            int stopwhen = -200;
            
            // FLAG POSITION: ALWAYS 0 at the start of packet.
            int FLAG_POSITION = 0;
                        /* The last acknowledgement did not receive */
            int LAD = 0;
            /* This is actually Last file send +1 */
            int LFS;
            
            /* The last packet read. */
            int LPR = 0;
            int ackCount[10] = {0};
            
            while (filelen > 0) {
                int stopwhen = -1;
                
                // Fill up the neccessary buffer to send, 5 maximum at a time.
                int move = 0;
                while ((move + LPR) < (LAD + SWS)) {
                    int packetPos = (move + LPR) % 10;
                    
                    // place a flag at the start of packet.
                    frames[packetPos][FLAG_POSITION] = packetPos;
                    
                    int read = 1;
                    while (read < BUFFER_SIZE) {
                        // Read the file into frame.
                        frames[packetPos][read] = fgetc(fileptr);
                        // Check EOF, end the loop.
                        if (frames[packetPos][read] == EOF) {
                            printf("%d", frames[packetPos][read]);
                            filelen = 0;
                            stopwhen = move + LPR;
                            break;
                        }
                        // FIle length -1.
                        filelen--;
                        read++;
                    }
                    move++;
                    if (frames[packetPos][read] == EOF) {
                        break;
                    }
                }
                
                // Renew the position of last packet read
                LPR += move;
                int sendNum = 0;
                while (LAD + sendNum < LPR) {
                    int packetNum = (sendNum + LAD) % 10;
                    
                    if (stopwhen == LAD + sendNum) {
                        sendto(sockfd, frames[packetNum],
                               sizeof(int) * BUFFER_SIZE, 0,
                               (struct sockaddr*) &clientaddr,
                               sizeof(clientaddr));
                        sendNum++;
                        break;
                    }

                    if (ackCount[packetNum] == 0) {
                        sendto(sockfd, frames[packetNum], sizeof(int) * BUFFER_SIZE, 0,
                               (struct sockaddr*) &clientaddr, sizeof(clientaddr));
                        LFS=LAD+SWS+sendNum;
                        sendNum++;
                    }else{
                        LAD++;
                    }
                    ackCount[(LFS)%10] = 0;
                }
                int run = 0;
                while (run < 5) {
                    int ack[1];
                    int checkACK = recvfrom(sockfd, ack, sizeof(int) * 1, 0,
                                            (struct sockaddr*) &clientaddr, &len);
                    
                    if ((LAR + 1) % 10 == ack[0]) {
                        LAR++;
                    }
                    printf("FLAG: %d\n", ack[0]);
                    ackCount[ack[0]] = 1;
                    run++;
                }
                if(LAR == LPR){
                    break;
                }
                LAD = LAR + 1;
            }
            fclose(fileptr);
        }
    }
    return 0;
}
