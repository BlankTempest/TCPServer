/*
 * tcpserver.c - A multithreaded TCP echo server
 * usage: tcpserver <port>
 *
 * Testing :
 * nc localhost <port> < input.txt
 */

#include "parallel.hpp"

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
        exit(1);
    }

    int val = 1;
    setsockopt(ssockid, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    if (bind(ssockid, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        cerr << "Error binding" << endl;
        exit(1);
    }

    if (listen(ssockid, maxcons) < 0)
    {
        cerr << "Error listening" << endl;
        exit(1);
    }

    for (int i = 0; i < threadCount; i++)
    {
        pthread_create(&pool_threads[i], NULL, threader, NULL);
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

        // handle_all(sclientid);
        pthread_mutex_lock(&mutex_queue);
        client_queue.push(sclientid);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex_queue);
    }

    close(ssockid);
    cleanup();

    return 0;
}