#include <wait.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include "logger.h"

file_logger *logger;

const int MAX_PATH_SIZE = 260;
const int MAX_PACKET_SIZE = 1024 * 1024;
const int MAX_MSG = 1000;
const char *restrincted_file_names = {
        ""
};

void *to_lower(char *p) {
    if (*p) {
        tolower(*p);
        to_lower(++p);
    }
}

bool is_path_valid(const char *path) {
    size_t path_len = strlen(path);
    if (path_len > MAX_PATH_SIZE && path_len <= 2) {
        return false;
    }

    char *lower_path = malloc((path_len + 1) * sizeof(char));
    bool is_valid = true;
    strcpy(lower_path, path);
    info(logger, path);
    info(logger, lower_path);
    to_lower(lower_path);

    for (size_t i = 0; i < path_len - 1; ++i) {
        if (path[i] == '.' && path[i + 1] == '.') {
            // tying to access parent directory
            is_valid = false;
            break;
        }
    }

    free(lower_path);
    return is_valid;
}

FILE *open_if_valid(const char *path) {
    if (is_path_valid(path)) {
        debug(logger, "File path is valid, opening file.");
        return fopen(path, "a");
    }

    debug(logger, "File path is invalid.");
    return NULL;
}

void write_packet(FILE *fp, const char *packet, uint32_t realSize) {
    if (fp != NULL) {
        fwrite(packet, sizeof(char), realSize, fp);
    }
}

void serve_client(int c) {
    char msg[MAX_MSG];
    uint32_t packetsCount, pathSize;
    char packet[MAX_PACKET_SIZE];
    char path[MAX_PATH_SIZE];

//    recv(c, msg, 10, MSG_WAITALL);
//    debug(logger, msg);


    recv(c, &packetsCount, sizeof(packetsCount), MSG_WAITALL);
    snprintf(msg, MAX_MSG, "Packets count: %d -> %d", packetsCount, ntohl(packetsCount));

    packetsCount = ntohl(packetsCount);
    info(logger, msg);

    recv(c, &pathSize, sizeof(pathSize), MSG_WAITALL);
    snprintf(msg, MAX_MSG, "Path length before conversion: %d", pathSize);
    info(logger, msg);
    pathSize = ntohl(pathSize);
    snprintf(msg, MAX_MSG, "Path length after conversion: %d", pathSize);
    info(logger, msg);

    memset(path, '\0', MAX_PATH_SIZE * sizeof(char));
    recv(c, path, pathSize * sizeof(char), MSG_WAITALL);
    snprintf(msg, MAX_MSG, "Received path '%s'", path);
    debug(logger, msg);
    FILE *fp = open_if_valid(path);

    for (int i = 0; i < packetsCount; i++) {
        uint32_t realSize = 0;
        recv(c, &realSize, sizeof(realSize), MSG_WAITALL);
        snprintf(msg, MAX_MSG, "Packet's %d size before conversion: %d", i, realSize);
        debug(logger, msg);
        realSize = ntohl(realSize);

        snprintf(msg, MAX_MSG, "Packet's %d real size: %d", i, realSize);
        debug(logger, msg);
        if (realSize > 0) {
            memset(packet, '\0', MAX_PACKET_SIZE * sizeof(char));
            recv(c, packet, realSize * sizeof(char), MSG_WAITALL);
            snprintf(msg, MAX_MSG, "Packet %d = '%s'", i, packet);
            debug(logger, msg);

            write_packet(fp, packet, realSize);
        } else {
            snprintf(msg, MAX_MSG, "Packet size is 0, proceeding...");
            debug(logger, msg);
        }

    }

    if (!is_path_valid(packet)) {
        snprintf(msg, MAX_MSG, "Error: Path \"%s\" is invalid", path);
    } else {
        snprintf(msg, MAX_MSG, "Success");
    }

    uint32_t response_size = (uint32_t) strlen(msg);
    printf("Sending response size %d -> %d\n", response_size, htonl(response_size));
    response_size = htonl(response_size);
    send(c, &response_size, sizeof(response_size), 0);
    send(c, msg, response_size, 0);
    printf("Sent message to client: %s\n", msg);

    if (fp != NULL) {
        snprintf(msg, MAX_MSG, "Closed file handle.");
        debug(logger, msg);
        fclose(fp);
    }
    close(c);
    info(logger, "Connection closed with client.");
}

void int_handler(int signal) {
    char msg[MAX_MSG];

    snprintf(msg, MAX_MSG, "Ctrl-C detected...exiting now.");
    info(logger, msg);
    destroy_logger(logger);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd, c;
    socklen_t len;
    char msg[MAX_MSG];
    struct sockaddr_in server, client;
    logger = new_logger("server.log");

    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);

    info(logger, "Created the logger");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        snprintf(msg, MAX_MSG, "Could not create the server socket: %s", strerror(errno));
        error(logger, msg);
        return 1;
    }
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        error(logger, "setsockopt(SO_REUSEADDR) failed");

    for (int i = 0; i < argc; i++) {
        snprintf(msg, MAX_MSG, "arg[%d] = %s", i, argv[i]);
        debug(logger, msg);
    }

    uint16_t port = 1024;
    if (argc > 1) {
        uint16_t newPort = (uint16_t) atoi(argv[1]);
        snprintf(msg, MAX_MSG, "Converted %s to %d", argv[1], newPort);
        debug(logger, msg);
        if (newPort != 0) {
            port = newPort;
        }
    }

    memset(&server, 0, sizeof(server));
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *) &server, sizeof(server))) {
        snprintf(msg, MAX_MSG, "Error when trying to bind: %s", strerror(errno));
        error(logger, msg);
        return 1;
    }
    snprintf(msg, MAX_MSG, "Listening for connections on port %d", port);
    info(logger, msg);

    listen(sockfd, 5);

    len = sizeof(client);
    memset(&client, 0, sizeof(client));

    while (1) {
        c = accept(sockfd, (struct sockaddr *) &client, &len);
        if (c < 0) {
            error(logger, "Could not accept connection");
        }
        snprintf(msg, MAX_MSG, "Connection accepted by server from %s.", inet_ntoa(client.sin_addr));
        info(logger, msg);

        if (fork() == 0) {
            serve_client(c);
            exit(0);
        }
    }
}