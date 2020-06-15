#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "HeaderFiles/Util.h"

using namespace std;


string serverIP;
int serverPort;

int threads;                            // Num of non ready threads
strList *query_list = new strList();    // List of all queries
pthread_mutex_t queryList_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t threadsReady_cond = PTHREAD_COND_INITIALIZER;


void* thread_function(void *arg) {
    int clientFD = socket(AF_INET, SOCK_STREAM, 0);
    if(clientFD < 0) {
        cerr << "- Error: Socket()\n\n";
        return NULL;
    }
    // Prepare for connetion
    sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    if(inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "- Error: inet_pton()\n\n";
        return NULL;
    }

    pthread_mutex_lock(&queryList_mutex);
    // Get queries from list
    int queriesForThread = query_list->length() / threads--;
    string queries[queriesForThread];
    for(int i = 0; i < queriesForThread; i++)
        queries[i] = query_list->dequeue();
    // If current thread is the last thread
    if(threads == 0)    // Signal threads to begin connections
        pthread_cond_broadcast(&threadsReady_cond);
    else                // Wait till all threads ready to start connections
        pthread_cond_wait(&threadsReady_cond, &queryList_mutex);

    pthread_mutex_unlock(&queryList_mutex);
    // Connect to server
    if(connect(clientFD, (sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "- Error: connect()\n\n";
        return NULL;
    }
    // Begin processing queries
    for(int i = 0; i < queriesForThread; i++) {
        if(sendMessage(clientFD, queries[i]) <= 0) {
            cout << "- Error: write() - Server disconnected\n\n";
            break;
        }
        // Receive result from server
        string res = receiveMessage(clientFD);
        if(res == END_READ) {
            cout << "- Error: read() - Server disconnected\n\n";
            break;
        }

        cout << queries[i] + "\n" + res + "\n";
    }
    
    shutdown(clientFD, SHUT_RDWR);
    close(clientFD);

    return NULL;
}


int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    
    string queryFile;
    int numThreads;

    if(argc < 9) {
        cerr << "- Error: Not Enough Parameters\n";
        return 1;
    }

    for(int i = 1; i < argc; i += 2) {
        string temp = argv[i];

        if(temp == "-q")
            queryFile = argv[i+1];
        else if(temp == "-sp")
            serverPort = atoi(argv[i+1]);
        else if(temp == "-w")
            numThreads = atoi(argv[i+1]);
        else if(temp == "-sip")
            serverIP = argv[i+1];
        else {
            cerr << "- Error: Invalid Parameter: " + temp + "\n";
            return 1;
        }
    }

    if(numThreads <= 0) {
        cerr <<  "- Error: At least 1 Worker needed\n";
        return 1;
    }

    threads = numThreads;

    string query;
    ifstream ifs(queryFile);
    // Read queries from file
    while(getline(ifs, query))
        query_list->add(query);
    // Initialize threads
    pthread_t thread_pool[numThreads];
    for(int i = 0; i < numThreads; i++)
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    // Wait for threads to complete
    for (int i = 0; i < numThreads; i++)
       pthread_join(thread_pool[i], NULL);
    // Clear allocated memory
    delete query_list;

    return 0;
}
