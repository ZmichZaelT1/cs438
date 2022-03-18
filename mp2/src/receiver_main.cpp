/* 
 * File:   receiver_main.c
 * Author: 
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define MSS 1300

typedef struct packet_ {
    long seqNum;
    int length;
    char data[MSS];
}packet;


struct sockaddr_in si_me, si_other;
int s, slen;
int expectedSeqNum = 1;
FILE *fp;
char* filename;

void writeToFile(FILE *fp, packet *pck) {
    fp = fopen(filename, "ab");
    fwrite(pck->data, sizeof(char), pck->length, fp);
    fclose(fp);
}

void diep(char *s) {
    perror(s);
    exit(1);
}



void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    filename = destinationFile;
    slen = sizeof (si_other);


    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");


	/* Now receive data and send acknowledgements */    
    // FILE *fp;
    // fp = fopen(destinationFile, "ab");
    // if (fp == NULL) {
    //     printf("Could not open file to write.");
    //     exit(1);
    // }

    while (1) {
        packet *pck = (packet*) calloc(1, sizeof(packet));
        socklen_t l = (socklen_t) slen;
        int bytes_read = recvfrom(s, pck, sizeof(*pck), 0, (struct sockaddr*) &si_other, &l);
        int receivedSeqNum = pck->seqNum;

        if (receivedSeqNum == -1) {
            break;
        }

        if (receivedSeqNum == expectedSeqNum) {
            writeToFile(fp, pck);
            expectedSeqNum++;
        }
        int sendArk = expectedSeqNum - 1;
        sendto(s, &sendArk, sizeof(sendArk), 0, (struct sockaddr*) &si_other, slen);
        // fflush (fp);

    }

    // fclose(fp);
    close(s);
	printf("%s received.", destinationFile);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

