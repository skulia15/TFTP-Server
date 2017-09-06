#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
/* your code goes here. */

int main(int argc, char *argv[])
{int sockfd;
    struct sockaddr_in server, client;
    char message[512];

    // Create and bind a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Receives should timeout after 30 seconds.
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Network functions need arguments in network byte order instead
    // of host byte order. The macros htonl, htons convert the
    // values.
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(32000);
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

    for (;;) {
        // Receive up to one byte less than declared, because it will
        // be NUL-terminated later.
        socklen_t len = (socklen_t) sizeof(client);
        ssize_t n = recvfrom(sockfd, message, sizeof(message) - 1,
                             0, (struct sockaddr *) &client, &len);
        if (n >= 0) {
            message[n] = '\0';
            fprintf(stdout, "Received:\n%s\n", message);
            fflush(stdout);

            // convert message to upper case.
            for (int i = 0; i < n; ++i) {
                message[i] = toupper(message[i]);
            }

            sendto(sockfd, message, (size_t) n, 0,
                   (struct sockaddr *) &client, len);
        } else {
            // Error or timeout. Check errno == EAGAIN or
            // errno == EWOULDBLOCK to check whether a timeout happened
        }
    }
	return 0;
}
