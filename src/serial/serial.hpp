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

#define tCount 10
#define maxcons 100

using namespace std;

// queue<int> client_queue;
unordered_map<string, string> KV_DATASTORE;

void handle_read(int sclientid, istringstream &streamstring)
{

    string input;
    streamstring >> input;

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

    return;
}

void handle_write(int sclientid, istringstream &streamstring)
{

    string key, value;

    streamstring >> key >> value;
    value = value.substr(1);

    KV_DATASTORE[key] = value;

    const char *msg = "FIN\n";

    write(sclientid, msg, strlen(msg));

    return;
}

void handle_count(int sclientid)
{

    int count = KV_DATASTORE.size();

    string se = to_string(count) + "\n";

    write(sclientid, se.c_str(), se.length());

    return;
}

void handle_delete(int sclientid, istringstream &streamstring)
{

    string input;
    streamstring >> input;

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

    write(sclientid, msg, strlen(msg));

    return;
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
    string strip = "";
    string input;

    while (true)
    {

        memset(sstream, 0, streamSize);
        ssize_t bytesRead = read(sclientid, sstream, streamSize - 1);

        if (bytesRead <= 0)
        {
            break;
        }

        istringstream streamstring(strip + (string)sstream);

        while (true)
        {

            streamstring >> input;
            if (input == "READ")
            {
                handle_read(sclientid, streamstring);
            }

            else if (input == "WRITE")
            {
                handle_write(sclientid, streamstring);
            }

            else if (input == "COUNT")
            {
                handle_count(sclientid);
            }

            else if (input == "DELETE")
            {
                handle_delete(sclientid, streamstring);
            }

            else if (input == "END")
            {
                handle_end(sclientid);
                return;
            }

            else
            {
                if (streamstring.str().empty())
                {
                    strip = input;
                    break;
                }
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