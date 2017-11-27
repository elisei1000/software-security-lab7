#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

const int MAX_PATH_SIZE = 260;
const int MAX_FILE_SIZE = 1024 * 1024 * 1024;
const int MAX_MSG = 1000;

const char  *reserved_files[] = {"con", "prn", "aux", "nul", "com1", "com2", "com3", "com4", "com5",
                          "com6", "com7", "com8", "com9", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5",
                          "lpt6", "lpt7", "lpt8", "lpt9"};
int reserved_files_number = 22;
uint16_t  server_port = 1024;

const char * server_address = "127.0.0.1";
char  firstPacket[271];
const size_t MAX_PATH = 260;
const int FIRST_PACKET_SIZE = 271;
const int FILE_PART_SIZE = 1024 * 1024;
const size_t PACKET_SIZE = 1024 * 1024 + 4;

void toLower(char * p){
    char *ch;
    ch = p;
    while((*ch) != 0){
        (*ch) = static_cast<char>(tolower(static_cast<unsigned char>(*ch)));
        ch++;
    }
}

bool is_valid_path(char * path){
    if(strstr(path, "..") != NULL)
        return false;

    if(strlen(path) > MAX_PATH)
        return false;

    for(int i=0;i<reserved_files_number; i++)
        if(strcmp(path, reserved_files[i]) == 0)
            return false;

    return true;
}


int main(int argc , char ** argv) {
    char *path;
    int client_socket;
    int packet_number, path_len, input_fd;
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

    if (buffer.st_size > MAX_FILE_SIZE){
        cout << "File is too big\n";
        return 0;
    }

    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Cannot create socket");
        return 0;
    }

    server = gethostbyname(server_address);
    if(server == NULL){
        cout<<"No such host: " << server_address<<"\n";
        return 0;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&(serv_addr.sin_addr), server->h_addr_list[0], (size_t)server->h_length);
    serv_addr.sin_port = htons(server_port);
    if(connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
        perror("Cannot connect to server");
        return 0;
    }

    cout<<"Connected to the server\n";
    cout<<"Sending file "<< path <<" of size " << buffer.st_size<<"\n";

    path_len = static_cast<int>(strlen(path)); // am verificat ca path-ul nu e mai lung de 260
    packet_number = ((int)buffer.st_size) / FILE_PART_SIZE + 1;

    memcpy(firstPacket, &packet_number, 4);
    memcpy(firstPacket+4, &path_len, 4);
    memcpy(firstPacket+8, path, 261);

    input_fd = open(path, O_RDONLY);
    if(input_fd == -1){
        perror("Can't open file: ");
        return 0;
    }

    cout<<"Sending first packet\n";
    if(write(client_socket, firstPacket, FIRST_PACKET_SIZE) != FIRST_PACKET_SIZE){
        perror("Can't send first packet to server:");
        return 0;
    }


    return 0;
}