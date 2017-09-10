#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

/* your code goes here. */

// Operation codes
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

// Error codes
#define NOT_DEFINED 0
#define FILE_NOT_FOUND 1
#define ACCESS_VIOLATION 2
#define DISK_FULL 3
#define ILLEGAL_OPERATION 4
#define UNKNOWN_TRANSFER_ID 5
#define FILE_ALREADY_EXISTS 6
#define NO_SUCH_USER 7 

// Constants
#define PACKET_SIZE 516 // Packets are 4B header and 512B blocks
#define DATA_BLOCK_SIZE 512 // Data is sent in 512B blocks
#define OPCODE_POS 1    // The operation code position in the message
#define OPCODE_SIZE 2 // The operation code is 2B in size 
#define PATH_MAX 255

FILE *fp = NULL;
unsigned short blockNo;
int sockfd;
struct sockaddr_in server, client;
int isLastPacket;
char *mode;
char dataPacket[PACKET_SIZE];

void closeFpIfOpen();
void setOpCode(int opcode, char * message);
void setBlockNumber(int blockNo, char * message);
void sendError(int errCode, char * errorMessage);
void sendDataPacket();
int handleRRQ(char * message, char * directory);
int handleACK(char * message, int n);
char * getMode(char * message, char * requestFile);

/*TODO: RETRANSMIT ON TIMEOUT*/
/*Downloading files outside of this directory must not be allowed.*/

// Fetches the mode from the given message
// Mode is found in the header, after the 2 opcode bytes, the filename and the null byte
char * getMode(char * message, char * requestFile){
    return message + (int)strlen(requestFile) + OPCODE_SIZE + 1;
}

// Closes the file pointer if it is open.
void closeFpIfOpen(){
    if(fp != NULL){
        fclose(fp);
    }
}

// Sets the operation code for the given packet. 
void setOpCode(int opcode, char * message){
    message[0] = 0;
    message[1] = opcode;
}

// Set the block number for the given packet,
// It can also set the error code for an error packet since the implementation is the same.
void setBlockNumber(int blockNo, char * message){
    message[2] = (blockNo >> 8);
    message[3] = (blockNo);
}

// Constructs an error packet and sends it to the client.
void sendError(int errCode, char * errorMessage){
    // Erase previous data from the data packet
    memset(dataPacket, 0 , sizeof(dataPacket));
    setOpCode(5, dataPacket);
    // The error code is set in the same way as the block number, so we use the same function
    setBlockNumber(errCode, dataPacket);
    // The error message is stored at byte 4
    strcpy(&dataPacket[4], errorMessage);
    // Send the error packet to the client
    int bytesSent = sendto(sockfd, dataPacket, (size_t)strlen(errorMessage) + (size_t)4 + (size_t)1 , 0, (struct sockaddr *)&client, (socklen_t)sizeof(client));
    if(bytesSent < 0){
        fprintf(stdout, "Failed to send error message \n");
        closeFpIfOpen();
        return;
    }
}

// Constructs a data packet by setting the opcode and the block number,
// reading data from the file to the buffer and then reading data from
// the buffer into the data packet and lastly sending the data packet to the client
void sendDataPacket(){
    // Buffer for the data we're about to send with this packet
    char dataBuffer[DATA_BLOCK_SIZE];

    // n is number of bytes read. Read 512B to the data buffer
    int n = 0;

    // Check which mode we got and read data depending on the mode
    if(strcmp(mode, "octet") == 0){
        n = fread(dataBuffer, 1, DATA_BLOCK_SIZE, fp);
    }
    else if(strcmp(mode, "netascii") == 0){
        n = fread(dataBuffer, sizeof(char), sizeof(dataBuffer)/sizeof(char), fp);
    }
    else{
        sendError(ILLEGAL_OPERATION, "This mode is not supported \n");
        return;
    }
    
    // If we successfully read some data from the file
    if(n >= 0){
        // Erase previous data from the data packet
        memset(dataPacket, 0, sizeof(dataPacket));

        // End condition Bytes received < 516
        if(n < DATA_BLOCK_SIZE){
            // This is the last packet of data
            isLastPacket = 1;
            closeFpIfOpen();
        }
        // Set the operation code
        setOpCode(DATA, dataPacket);

        // Set the block number
        setBlockNumber(blockNo, dataPacket);

        // Move the data read from the data buffer to the Data packet
        for(int i = 0; i < n; i++){
            dataPacket[i + 4] = dataBuffer[i];
        }
        
        // Send the packet to the client. Size is the size of the data + 4 bytes for the opcode and block number.
        int bytesSent = sendto(sockfd, dataPacket, (size_t)n + (size_t)4, 0, (struct sockaddr *)&client, (socklen_t) sizeof(client));
        if (bytesSent < 0){
            sendError(NOT_DEFINED, "Failed to send Data to client\n");
            return;
        }
    }
}

int handleRRQ(char * message, char * directory){
    blockNo = 1;
    // The isLastPacket packet is not yet received
    isLastPacket = 0;
    
    // Get the name of the requested file.
    char *requestFile = message + OPCODE_SIZE;

    // Construct the full path
    char path[PATH_MAX];
    strcpy(path, directory);
    strcat(path, "/");
    strcat(path, requestFile);

    // Find out the mode requested
    mode = getMode(message, requestFile);

    fprintf(stdout, "file \"%s\" requested from ", requestFile);
    //fprintf(stdout, "IP %s \n", inet_ntoa(server.sin_addr));
    //fprintf(stdout, "Port %s \n", server.sin_port);
    //file "example_data1" requested from 127.0.0.1:37242

    // Open the file
    fp = fopen(path, "rb");
    if(fp == NULL){
        sendError(FILE_NOT_FOUND, "Failed to open the given file");
        return -1;
    }
    sendDataPacket();
    return 1;
}

int handleACK(char * message, int n){
    if(isLastPacket){
        fprintf(stdout, "Transmission received\n");
        return 1;
    }

    // The acknowledgment packet's block number is found in the third and fourth byte if the packet
    unsigned short ackBlockNo = (((unsigned char*)message)[2] << 8) + ((unsigned char*)message)[3];

    // Check if acknowledgement block number corresponds to our block number
    if(ackBlockNo == blockNo){
        blockNo++;
        sendDataPacket();
    }
    // If client did not receive the previous data package
    else if(ackBlockNo == (blockNo - 1)){
        // Resend the data package
        int sent = sendto(sockfd, dataPacket, (size_t)n + (size_t)4, 0, (struct sockaddr *)&client, (socklen_t)sizeof(client));
        if(sent <= 0){
            sendError(NOT_DEFINED, "Failed to resend Data package\n");
            return -1;
        }
    }
    // Send error if block number is ridiculous
    else{
        sendError(NOT_DEFINED, "Mismatched block number in acknowledgement\n");
        return -1;
    }
    return 1;
}

int main(int argc, char *argv[]){
    // Program takes 2 arguments
    if(argc != 3){
        fprintf(stdout, "The program requires 2 arguments\n");
        return -1;
    }
    
    // The received message
    char message[DATA_BLOCK_SIZE];
    char *directory = argv[2];

    // Create and bind a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1){
		fprintf(stdout, "Failed to create the socket\n");
		return -1;
	}

    // Network functions need arguments in network byte order instead
    // of host byte order. The macros htonl, htons convert the
    // values.
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(atoi(argv[1]));
    bind(sockfd, (struct sockaddr *)&server, (socklen_t)sizeof(server));

    for (;;) {
        // Receive up to one byte less than declared, because it will
        // be NUL-terminated later.
        socklen_t len = (socklen_t)sizeof(client);
        ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1, 0, (struct sockaddr *)&client, &len);
        // Failed to receive from socket
        if(n < 0){
            sendError(NOT_DEFINED, "Failed to receive data from the socket");
        }

        // Add a null-terminator to the message
        message[n] = '\0';

        // Process the received message
        short opcode = (short)message[OPCODE_POS];

        switch(opcode){
            case RRQ:
                if(handleRRQ(message, directory) == -1){
                    return -1;
                }
                break;
            case WRQ:
                // Not implemented
                sendError(ILLEGAL_OPERATION, "Write requests are not allowed\n");
                break;
            case DATA:
                // Cannot receive data packet from client
                sendError(ILLEGAL_OPERATION, "Data upload is not allowed\n");
                break;
            case ACK:
                // Do Acknowledgement
                if(handleACK(message, n) == -1){
                    return -1;
                }
                break;
            default:
                sendError(ILLEGAL_OPERATION, "Operation code is not valid");
        }  
    }

	// Close the socket
	if(close(sockfd) != 0){
		fprintf(stdout, "Closing the socket failed\n");
		return -1;
	}
    fprintf(stdout, "END OF PROGRAM\n");
	return 0;
}
