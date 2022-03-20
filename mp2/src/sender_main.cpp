/* 
 * File:   sender_main.c
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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <map>
#include <iostream>

using namespace std;

#define MSS 1024
#define TIME_TH 160000 //us

struct sockaddr_in si_other;
int s, slen;
int base = 1;
int nextSeqNum = 1;
int highestAckReceived = 0;
int num_packets = 0;
// double time_th = 1;
int N = 1;

typedef struct packet_ {
    long seqNum;
    int length;
    char data[MSS];
}packet;

map <int, packet*> packets_map;

void finishedSending() {
    for (int i = 0; i < 5; i++) {
        int end = -1;
        sendto(s, &end, sizeof(end), 0, (struct sockaddr*) &si_other, sizeof(si_other));
    }
}

void loadFileToPacket(FILE *fp, unsigned long long int bytesToTransfer) {
    num_packets = bytesToTransfer / MSS;
    if (bytesToTransfer % MSS != 0) num_packets++;
    int count = 1;
    int total_bytes_read = 0;
    while (!feof(fp)) {
        packet *pck = (packet*) calloc(1, sizeof(packet));
        // pck->data = (char*) calloc(MSS, sizeof(char));
        size_t bytes_read = fread(pck->data, sizeof(char), MSS, fp);
        if (bytes_read <= 0) {
            free(pck->data);
            free(pck);
            break;
        }
        pck->seqNum = count;
        pck->length = (int) bytes_read;
        packets_map[count++] = pck;
        total_bytes_read += bytes_read;
        // count++;
    }
    cout << "total byte read: "<< total_bytes_read<<"\ntotal packets: "<<count<<endl; 
}

int checkReceive() {
    int ack = -1;
    socklen_t len = sizeof(si_other);
    recvfrom(s, &ack, sizeof(ack), MSG_DONTWAIT, (struct sockaddr*) &si_other, &len);
    return ack;
}

void diep(char *s) {
    perror(s);
    exit(1);
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }
    loadFileToPacket(fp, bytesToTransfer);
    fclose(fp);

	/* Determine how many bytes to transfer */

    slen = sizeof (si_other);
///////////////////// init socket ////////////////// 
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIME_TH;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
//////////////////////////////////////////////////// 

/////////
    // int dropPacket = 1;
    // int dropIndex = 2;
//////////
    while (1) {
        if (highestAckReceived == num_packets) break;
        int tmp = nextSeqNum;
        int sentCount = 0;
        cout<< "window size: " << N << endl;
        for (int i = 0; i < N; i++) {
            if (nextSeqNum > num_packets) {
                break;
            }
            /////////////////////////
            // if (dropPacket && dropIndex == i) {
            //     dropPacket = 0;
            //     nextSeqNum++;
            //     continue;
            // }
            /////////////////////////
            packet *toSend = packets_map[nextSeqNum];
            sendto(s, toSend, sizeof(*toSend), 0, (struct sockaddr*)&si_other, sizeof(si_other));
            nextSeqNum++;
            sentCount++;
        }
        cout << "sended " << tmp << " - " << nextSeqNum-1 << endl;
        // clock_t start = clock();

        int expectAck = nextSeqNum - 1;
        int prevAck = -2;
        int countDupAck = 0;
        int halfed = 0;
        // while ((double) (clock()-start) / (double) CLOCKS_PER_SEC < 1) {
        for (int i = 0; i < sentCount; i++) {
            if (highestAckReceived == num_packets) break;
            // int newAck = checkReceive();
            int newAck = -1;
            socklen_t len = sizeof(si_other);
            if (recvfrom(s, &newAck, sizeof(newAck), 0, (struct sockaddr*) &si_other, &len) != sizeof(int)) {
                nextSeqNum = highestAckReceived + 1;
                N = 1;
                break;
            }

            if (newAck == expectAck) {
                highestAckReceived = newAck;
                nextSeqNum = highestAckReceived + 1;
                break;
            }

            if (newAck > highestAckReceived) {
                highestAckReceived = newAck;
                nextSeqNum = highestAckReceived + 1;
            }
            
            if (newAck == prevAck) {
                countDupAck++;
                if (countDupAck == 3 && halfed == 0) {
                    N = N / 2;
                }
            }
            prevAck = newAck;
            // cout << "received ack: " << newAck<< "     highestAck: "<<highestAckReceived<< endl;
        }
        cout << "highestAck: " << highestAckReceived<< endl;

        // if (nextSeqNum == tmp) { // no ack received
        //     nextSeqNum = tmp; // reset nextSeqNum
        // }
        if (halfed == 0) {
            N *= 2;
        }
        // N *= 4;
    }


    finishedSending();

	/* Send data and receive acknowledgements on s*/

    printf("Closing the socket\n");
    close(s);
    return;

}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}
