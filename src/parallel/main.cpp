/* 
 * tcpserver.c - A multithreaded TCP echo server 
 * usage: tcpserver <port>
 * 
 * Testing : 
 * nc localhost <port> < input.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sstream>
#include <arpa/inet.h>
#include <unordered_map>
#include <map>
#include <queue>
#include <vector>
#include <csignal>

#define threadCount 10
#define maxcons 100

using namespace std;

pthread_t pool_threads[threadCount];
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_dict = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

queue<int> client_queue;
unordered_map<string,string> KV_DATASTORE;


void enqueuer(queue<int> &cq, int ele) {
	cq.push(ele);

	return;
}

void dequeuer(queue<int> &cq) {
	if (!cq.empty()) {
        cq.pop();
	}

	return;
}

void handle_read(int sclientid, istringstream& streamstring) {
    string input;
    streamstring >> input;

	pthread_mutex_lock(&mutex_dict);

    auto it = KV_DATASTORE.find(input);
	if (it == KV_DATASTORE.end()) {
		const char *msg = "NULL\n";
		write(sclientid, msg, strlen(msg));
	}
	
	else {
		const char *msg = (it->second + "\n").c_str();
		write(sclientid, msg, strlen(msg));
	}

	pthread_mutex_unlock(&mutex_dict);

	return;
}

void handle_write(int sclientid, istringstream& streamstring) {
    string key, value;

    streamstring >> key >> value;
    value = value.substr(1);

	pthread_mutex_lock(&mutex_dict);

    KV_DATASTORE[key] = value;

	pthread_mutex_unlock(&mutex_dict);

    const char *msg = "FIN\n";

    write(sclientid, msg, strlen(msg));

	return;
}

void handle_count(int sclientid) {

	pthread_mutex_lock(&mutex_dict);

    int count = KV_DATASTORE.size();

	pthread_mutex_unlock(&mutex_dict);

    string se = to_string(count) + "\n";

    write(sclientid, se.c_str(), se.length());

	return;
}

void handle_delete(int sclientid, istringstream& streamstring) {
    string input;
    streamstring >> input;

	pthread_mutex_lock(&mutex_dict);

    auto it = KV_DATASTORE.find(input);
    const char* msg;

    if (it == KV_DATASTORE.end()) {
        msg = "NULL\n";
    }
	
	else {
        KV_DATASTORE.erase(it);
        msg = "FIN\n";
    }

	pthread_mutex_unlock(&mutex_dict);

    write(sclientid, msg, strlen(msg));

	return;
}

void handle_end(int sclientid) {
    write(sclientid, "\n", 1);
    close(sclientid);

	return;
}

void handle_all(int sclientid) {
    constexpr size_t streamSize = 2048;
    char sstream[streamSize];
    string strip = "";
    string input;

    while (true) {
        memset(sstream, 0, streamSize);
        ssize_t bytesRead = read(sclientid, sstream, streamSize-1);
        if (bytesRead <= 0) {
            break;
        }
		istringstream streamstring(strip + (string)sstream);

        while (true) {
            streamstring >> input;
            if (input == "READ") {
                handle_read(sclientid, streamstring);
            }
			
			else if (input == "WRITE") {
                handle_write(sclientid, streamstring);
            }
			
			else if (input == "COUNT") {
                handle_count(sclientid);
            }
			
			else if (input == "DELETE") {
                handle_delete(sclientid, streamstring);
            }
			
			else if (input == "END") {
                handle_end(sclientid);
                return;
            }
			
			else {
                if (streamstring.str().empty()) {
                    strip = input;
                    break;
                }
            }

            if (streamstring.str().empty()) {
                strip = "";
                break;
            }
        }
    }
	
	close(sclientid);

	return;
}

void* threader(void* john_smith) {
	
	john_smith = NULL;

	while (true==true) {

		int sclientid;

		pthread_mutex_lock(&mutex_queue);

		while (client_queue.empty()) {
			pthread_cond_wait(&cond,&mutex_queue);
		}

		sclientid = client_queue.front();
		client_queue.pop();

		pthread_mutex_unlock(&mutex_queue);

		handle_all(sclientid);
	}
	
	return NULL;
}

void cleanup() {
    for (int i = 0; i < threadCount; i++) {
        pthread_join(pool_threads[i], NULL);
    }

	return;
}

void signal_handle(int signum) {
    if (signum == SIGINT) {
        cleanup();
        std::cerr << "Exiting..\n";
        exit(0);
    }

	return;
}

int main(int argc, char ** argv) {
	
	int portno;
	if (argc != 2) {
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
    if (ssockid < 0) {
    	cerr << "Error creating" << endl;
    	exit(1);
    }

	int val = 1;
    setsockopt(ssockid, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    
    if (bind(ssockid, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
    	cerr << "Error binding"<<endl;
    	exit(1);
    }
	
	if (listen(ssockid, maxcons) < 0) {
        cerr << "Error listening"<<endl;
        exit(1);
    }

	for (int i=0; i< threadCount; i++) {
    	pthread_create(&pool_threads[i], NULL, threader, NULL);
	}

    while (true==true) {
        int sclientid;
        struct sockaddr_in clientAddress;
        socklen_t sclientlen = sizeof(clientAddress);
        
        sclientid = accept(ssockid, (struct sockaddr*) &clientAddress, &sclientlen);
        if(sclientid < 0) {
            cerr << "Error accepting from client"<<endl;
            continue;
        }

        //handle_all(sclientid);
		pthread_mutex_lock(&mutex_queue);
		client_queue.push(sclientid);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex_queue);
    }

    close(ssockid);
	cleanup(); 

	return 0;
}