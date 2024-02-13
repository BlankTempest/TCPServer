/*
 * tcpserver.c - A multithreaded TCP echo server
 * usage: tcpserver <port>
 *
 * Testing :
 * nc localhost <port> < input.txt
 */

#include "serial.hpp"

using namespace std;

int main(int argc, char **argv)
{

    int portno;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // DONE: Server port number taken as command line argument
    portno = atoi(argv[1]);

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portno);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int ssockid = socket(AF_INET, SOCK_STREAM, 0);

    if (ssockid < 0)
    {
        cerr << "Error creating" << endl;
        exit(0);
    }

    if (bind(ssockid, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        cerr << "Error binding" << endl;
        exit(0);
    }

    if (listen(ssockid, 100) < 0)
    {
        cerr << "Error listening" << endl;
        exit(0);
    }

    while (true == true)
    {

        int sclientid;
        struct sockaddr_in clientAddress;
        socklen_t sclientlen = sizeof(clientAddress);

        sclientid = accept(ssockid, (struct sockaddr *)&clientAddress, &sclientlen);
        if (sclientid < 0)
        {
            cerr << "Error accepting from client" << endl;
            continue;
        }

        handle_all(sclientid);
    }

    close(ssockid);

    return 0;
}