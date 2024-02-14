/*
 * tcpserver.c - A multithreaded TCP echo server
 * usage: tcpserver <port>
 *
 * Testing :
 * nc localhost <port> < input.txt
 */

#include "parallel.hpp"

Parallel::Server server;

void *threader(void *john_smith)
    {

        john_smith = NULL;

        while (true == true)
        {

            int sclientid;

            pthread_mutex_lock(&server.mutex_queue);

            while (server.client_queue.empty())
            {
                pthread_cond_wait(&server.cond, &server.mutex_queue);
            }

            sclientid = server.client_queue.front();
            server.client_queue.pop();

            pthread_mutex_unlock(&server.mutex_queue);

            server.handle_all(sclientid);
        }

        return NULL;
    }

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
        std::cerr << "Error creating" << std::endl;
        exit(1);
    }

    int val = 1;
    setsockopt(ssockid, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    if (bind(ssockid, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        std::cerr << "Error binding" << std::endl;
        exit(1);
    }

    if (listen(ssockid, maxcons) < 0)
    {
        std::cerr << "Error listening" << std::endl;
        exit(1);
    }

    for (int i = 0; i < threadCount; i++)
    {
        pthread_create(&server.pool_threads[i], NULL, threader, NULL);
    }

    while (true == true)
    {
        int sclientid;
        struct sockaddr_in clientAddress;
        socklen_t sclientlen = sizeof(clientAddress);

        sclientid = accept(ssockid, (struct sockaddr*)&clientAddress, &sclientlen);
        if (sclientid < 0)
        {
            std::cerr << "Error accepting from client" << std::endl;
            continue;
        }

        // handle_all(sclientid);
        pthread_mutex_lock(&server.mutex_queue);
        server.client_queue.push(sclientid);
        pthread_cond_signal(&server.cond);
        pthread_mutex_unlock(&server.mutex_queue);
    }

    close(ssockid);
    server.cleanup();

    return 0;
}