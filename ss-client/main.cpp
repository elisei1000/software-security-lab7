#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>

using namespace std;

const int MAX_PATH_SIZE = 260;
const int MAX_FILE_SIZE = 1024 * 1024 * 1024;
const int MAX_MSG = 1000;

const char *reserved_files[] = {"con", "prn", "aux", "nul", "com1", "com2", "com3", "com4", "com5",
                                "com6", "com7", "com8", "com9", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5",
                                "lpt6", "lpt7", "lpt8", "lpt9"};
int reserved_files_number = 22;
uint16_t server_port = 1026;

const char *server_address = "127.0.0.1";
const size_t MESSAGE_LENGTH = 255;
const size_t MAX_PATH = 260;
const uint32_t FILE_PART_SIZE = 10;
const uint32_t PACKET_SIZE = FILE_PART_SIZE + 4;
char firstPacket[271], file_packet[FILE_PART_SIZE], message[MESSAGE_LENGTH + 1];

void toLower(char *p) {
    char *ch;
    ch = p;
    while ((*ch) != 0) {
        (*ch) = static_cast<char>(tolower(static_cast<unsigned char>(*ch)));
        ch++;
    }
}

bool is_valid_path(char *path) {
    if (strstr(path, "..") != NULL)
        return false;

    if (strlen(path) > MAX_PATH)
        return false;

    for (int i = 0; i < reserved_files_number; i++)
        if (strcmp(path, reserved_files[i]) == 0)
            return false;

    return true;
}


int main(int argc, char **argv) {
    char *path;
    int client_socket;
    int input_fd;
    uint32_t translated;
    uint32_t path_len, packet_number;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    struct stat buffer;

    if (argc != 2) {
        cout << "Usage " << argv[0] << " filename\n";
        return 0;
    }

    path = argv[1];
    if (!is_valid_path(path)) {
        cout << "Invalid path provided!\n";
        return 0;
    }
    cout << path << "\n";
    if (lstat(path, &buffer) == -1) {
        perror("Cannot access file");
        return 0;
    }

    if (buffer.st_size > MAX_FILE_SIZE) {
        cout << "File is too big\n";
        return 0;
    }

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Cannot create socket");
        return 0;
    }

    server = gethostbyname(server_address);
    if (server == NULL) {
        cout << "No such host: " << server_address << "\n";
        return 0;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&(serv_addr.sin_addr), server->h_addr_list[0], (size_t) server->h_length);
    serv_addr.sin_port = htons(server_port);
    if (connect(client_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
        perror("Cannot connect to server");
        return 0;
    }

    cout << "Connected to the server\n";
    cout << "Sending file " << path << " of size " << buffer.st_size << "\n";

    path_len = static_cast<int>(strlen(path)); // am verificat ca path-ul nu e mai lung de 260
    packet_number = static_cast<uint32_t>(ceil(buffer.st_size / FILE_PART_SIZE)) + 1;
    translated = htonl(packet_number);
    cout << "packet number " << packet_number << "->" << htonl(packet_number) << endl;

    send(client_socket, &translated, sizeof(translated), 0);
    translated = htonl(path_len);
    send(client_socket, &translated, sizeof(translated), 0);

    cout << "path len" << path_len << endl;
    cout << "sending path " << path << endl;
    send(client_socket, path, path_len * sizeof(char), 0);

    input_fd = open(path, O_RDONLY);
    if (input_fd == -1) {
        perror("Can't open file: ");
        return 0;
    }

    cout << "Sending packets...\n";

    for (int i = 0; i < packet_number; ++i) {
        cout << "Sending packet #" << i << "\n";
        memset(file_packet, '\0', FILE_PART_SIZE);
        uint32_t bytesRead = static_cast<uint32_t>(pread(input_fd, file_packet, FILE_PART_SIZE, FILE_PART_SIZE * i));
        if (bytesRead == -1) {
            perror("Failed to read file: ");
            return 0;
        }
        if (bytesRead == 0) {
            continue;
        }

        cout << "bytes read " << bytesRead << "->" << htonl(bytesRead) << endl;
        cout << "==========" << endl << file_packet << endl << "===========" << endl;
        translated = htonl(bytesRead);
        send(client_socket, &translated, sizeof(translated), 0);
        send(client_socket, file_packet, bytesRead, 0);
    }

    memset(message, '\0', 256);
    uint32_t message_length = 0;
    if (recv(client_socket, &message_length, sizeof(message_length), MSG_WAITALL) != 4) {
        perror("Can't receive message!");
        return 0;
    }
    cout << "received meg length " << message_length << " -> " << ntohl(message_length) << endl;
    message_length = ntohl(message_length);

    message_length = min(message_length, (uint32_t) MESSAGE_LENGTH);
    cout << "Message length: " << message_length << endl;
    recv(client_socket, message, message_length, MSG_WAITALL);
    cout << "Received message " << message;
    return 0;
}