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

namespace Parallel 
{

    class Operations 
    {
        private: 
            pthread_mutex_t mutex_dict = PTHREAD_MUTEX_INITIALIZER;
            std::unordered_map<std::string, std::string> KV_DATASTORE;
        
        public:

            void handle_read(int sclientid, std::istringstream &streamstring)
            {
                std::string input;
                streamstring >> input;

                pthread_mutex_lock(&mutex_dict);

                auto it = KV_DATASTORE.find(input);
                if (it == KV_DATASTORE.end())
                {
                    const char *msg = "NULL\n";
                    write(sclientid, msg, strlen(msg));
                }

                else
                {
                    const char *msg = (it->second + "\n").c_str();
                    write(sclientid, msg, strlen(msg));
                }

                pthread_mutex_unlock(&mutex_dict);

                return;
            }

            void handle_write(int sclientid, std::istringstream &streamstring)
            {
                std::string key, value;

                streamstring >> key >> value;
                value = value.substr(1);

                pthread_mutex_lock(&mutex_dict);

                KV_DATASTORE[key] = value;

                pthread_mutex_unlock(&mutex_dict);

                const char *msg = "FIN\n";

                write(sclientid, msg, strlen(msg));

                return;
            }

            void handle_count(int sclientid)
            {

                pthread_mutex_lock(&mutex_dict);

                int count = KV_DATASTORE.size();

                pthread_mutex_unlock(&mutex_dict);

                std::string se = std::to_string(count) + "\n";

                write(sclientid, se.c_str(), se.length());

                return;
            }

            void handle_delete(int sclientid, std::istringstream &streamstring)
            {
                std::string input;
                streamstring >> input;

                pthread_mutex_lock(&mutex_dict);

                auto it = KV_DATASTORE.find(input);
                const char *msg;

                if (it == KV_DATASTORE.end())
                {
                    msg = "NULL\n";
                }

                else
                {
                    KV_DATASTORE.erase(it);
                    msg = "FIN\n";
                }

                pthread_mutex_unlock(&mutex_dict);

                write(sclientid, msg, strlen(msg));

                return;
            }
    };

    class Server
    {
        public:
            pthread_t pool_threads [threadCount];
            pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
            pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
            std::queue <int> client_queue;
            Operations operation;

        public:
            Server() {
                /*
                for (int i = 0; i < threadCount; i++)
                {
                    pthread_create(&server.pool_threads[i], NULL, threader, NULL);
                }
                */
            }
            ~Server() {
                //cleanup();
            }

            void enqueuer(int ele)
            {
                pthread_mutex_lock(&mutex_queue);
                client_queue.push(ele);
                pthread_cond_signal(&cond);
                pthread_mutex_unlock(&mutex_queue);
            }
            
            void dequeuer()
            {
                pthread_mutex_lock(&mutex_queue);
                if (!client_queue.empty())
                {
                    client_queue.pop();
                }
                pthread_mutex_unlock(&mutex_queue);
            }
            
            void handle_end(int sclientid)
            {
                write(sclientid, "\n", 1);
                close(sclientid);

                return;
            }

            void handle_all(int sclientid)
            {
                constexpr size_t streamSize = 2048;
                char sstream[streamSize];
                std::string strip = "";
                std::string input;

                while (true)
                {
                    memset(sstream, 0, streamSize);
                    ssize_t bytesRead = read(sclientid, sstream, streamSize - 1);
                    if (bytesRead <= 0)
                    {
                        break;
                    }
                    std::istringstream streamstring(strip + (std::string)sstream);

                    Operations operation;

                    while (true)
                    {
                        streamstring >> input;
                        
                        int flag = handle_switch(sclientid,operation,input,streamstring,strip);

                        if (flag == 1) {
                            break;
                        }
                        if(flag == 2) {
                            return;
                        }

                        if (streamstring.str().empty())
                        {
                            strip = "";
                            break;
                        }
                    }
                }

                close(sclientid);

                return;
            }

            void cleanup()
            {
                for (int i = 0; i < threadCount; i++)
                {
                    pthread_join(pool_threads[i], NULL);
                }

                return;
            }

            void signal_handle(int signum)
            {
                if (signum == SIGINT)
                {
                    cleanup();
                    std::cerr << "Exiting..\n";
                    exit(0);
                }

                return;
            }
        
        private:
            bool handle_switch(int sclientid, Operations &operation, const std::string &input, 
            std::istringstream &streamstring, std::string &strip) 
            {
                switch (get_operation_type(input)) 
                {
                    case OperationType::READ:
                        operation.handle_read(sclientid, streamstring);
                    return 0;

                    case OperationType::WRITE:
                        operation.handle_write(sclientid, streamstring);
                    return 0;

                    case OperationType::COUNT:
                        operation.handle_count(sclientid);
                    return 0;

                    case OperationType::DELETE:
                        operation.handle_delete(sclientid, streamstring);
                    return 0;

                    case OperationType::END:
                        handle_end(sclientid);
                    return 2;

                    default:
                        if (streamstring.str().empty())
                        {
                            strip = input;
                            return 1;
                        }
                    return 0;
                    
                }
            }

            enum class OperationType 
            {
                READ,
                WRITE,
                COUNT,
                DELETE,
                END,
                INVALID
            };

            OperationType get_operation_type(const std::string &input) 
            {
                if (input == "READ") 
                {
                    return OperationType::READ;
                }
                else if (input == "WRITE") 
                {
                    return OperationType::WRITE;
                }
                else if (input == "COUNT") 
                {
                    return OperationType::COUNT;
                }
                else if (input == "DELETE") 
                {
                    return OperationType::DELETE;
                }
                else if (input == "END") 
                {
                    return OperationType::END;
                }
                else 
                {
                    return OperationType::INVALID;
                }
            }
    };
}