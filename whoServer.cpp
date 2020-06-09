#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <dirent.h>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "HeaderFiles/Util.h"
#include "HeaderFiles/ServerUtil.h"

using namespace std;


intList *workersQueryFD_list = new intList();

workerList *workers = new workerList();
pthread_mutex_t workers_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t workers_cond = PTHREAD_COND_INITIALIZER;

intList *clients = new intList();
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clients_cond = PTHREAD_COND_INITIALIZER;

ringBuffer *buffer;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_push_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_pop_cond = PTHREAD_COND_INITIALIZER;


void handleWorker(int fd) {
    int error = 0;
    socklen_t len = sizeof(error);
    // while worker active keep fd in thread
    while(getsockopt (fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0) {
        string stats = receiveMessage(fd);  // Receive query response from worker
        // Wait till workers list is thread safe
        pthread_mutex_lock(&workers_mutex);
        // Insert query response in data struct
        workers->addData(fd, stats);
        // release mutex and signal that worker data updated
        pthread_cond_signal(&workers_cond);
        pthread_mutex_unlock(&workers_mutex);
    }
}

void handleClient(int fd) {
    string query = receiveMessage(fd);  // Receive query from client
    // lock client to prevent new worker connections
    pthread_mutex_lock(&clients_mutex);
    // Send query to workers
    for(int i = 0; i < workersQueryFD_list->length(); i++)
        sendMessage(workersQueryFD_list->get(i), query);
    // Add client fd to data struct with requested query
    clients->add(fd);
    // Wait for client's turn receive results
    while(fd != clients->get(clients->length() - 1))
        pthread_cond_wait(&clients_cond, &clients_mutex);
    // Client no longer in queue
    clients->dequeue();
    // Lock workers when stats are ready
    pthread_mutex_lock(&workers_mutex);
    while(!workers->allHaveData())
        pthread_cond_wait(&workers_cond, &workers_mutex);
    // Compose query answer
    string res = "";
    if(query.find("/diseaseFrequency") == string::npos) {
        for(int i = 0; i < workers->length(); i++)
            res += workers->removeDatafromIndex(i);
        // Result not found
        if(res == "") res = "--Not found\n";
    }
    else {
        int temp = 0;
        for(int i = 0; i < workers->length(); i++)
            temp += stoi(workers->removeDatafromIndex(i));

        res = to_string(temp) + "\n";
    }
    // Signal clients change and release mutexes
    pthread_mutex_unlock(&workers_mutex);
    pthread_cond_signal(&clients_cond);
    pthread_mutex_unlock(&clients_mutex);
    // Print and send query answer
    cout << query + "\n" + res + "\n";
    sendMessage(fd, res);
}


void* thread_function(void *arg) {
    while(true) {
        // Wait until there is an fd in buffer
        pthread_mutex_lock(&buffer_mutex);
        while(buffer->isEmpty())
            pthread_cond_wait(&buffer_push_cond, &buffer_mutex);
        // Get fd from buffer
        int fd = buffer->pop();
        pthread_cond_signal(&buffer_pop_cond);
        pthread_mutex_unlock(&buffer_mutex);
        // Check if fd belongs to worker connection
        pthread_mutex_lock(&workers_mutex);
        bool isWorker = workers->member(fd);
        pthread_mutex_unlock(&workers_mutex);
        // Handle fd
        if(isWorker)
            handleWorker(fd);
        else
            handleClient(fd);
        
        close(fd);
    }
}


int main(int argc, char* argv[]) {
    int bufferSize, queryPortNum, statisticsPortNum, numThreads;

    if(argc < 9) {
        cerr << "- Error: Not Enough Parameters\n";
        return 1;
    }
    // Get server parameters
    for(int i = 1; i < argc; i += 2) {
        string temp = argv[i];

        if(!temp.compare("-q"))
            queryPortNum = atoi(argv[i+1]);
        else if(!temp.compare("-s"))
            statisticsPortNum = atoi(argv[i+1]);
        else if(!temp.compare("-w"))
            numThreads = atoi(argv[i+1]);
        else if(!temp.compare("-b"))
            bufferSize = atoi(argv[i+1]);
        else {
            cerr << "- Error: Invalid Parameter: " + temp + "\n";
            return 1;
        }
    }

    if(numThreads <= 0) {
        cerr <<  "- Error: At least 1 Worker needed\n";
        return 1;
    }
    if(bufferSize <= 0) {
        cerr <<  "- Error: Buffer size too small\n";
        return 1;
    }
    // Init ring buffer
    buffer = new ringBuffer(bufferSize);

    // Create and init thread pool
    pthread_t thread_pool[numThreads];
    for(int i = 0; i < numThreads; i++)
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);

    // Create socket for worker connections
    int listenStatsFD = socket(AF_INET, SOCK_STREAM, 0);
    if(listenStatsFD < 0) {
        cerr <<  "- Error: socket()\n";
        return 1;
    }
    
    sockaddr_in statsServerAddr;
    bzero(&statsServerAddr, sizeof(statsServerAddr));
    
    statsServerAddr.sin_family = AF_INET;
    statsServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    statsServerAddr.sin_port = htons(statisticsPortNum);
    // Reuse socket if not closed properly
    int reuseStatsPort = 1;
    if (setsockopt(listenStatsFD, SOL_SOCKET, SO_REUSEADDR, &reuseStatsPort, sizeof(reuseStatsPort)) < 0) {
        cerr <<  "- Error: setsockopt()\n";
        return 1;
    }
    if(bind(listenStatsFD, (sockaddr*) &statsServerAddr, sizeof(statsServerAddr)) < 0) {
        cerr <<  "- Error: bind()\n";
        return 1;
    }
    if(listen(listenStatsFD, 10) < 0) {
        cerr <<  "- Error: listen()\n";
        return 1;
    }
    // Create socket for client connections
    int listenQueryFD = socket(AF_INET, SOCK_STREAM, 0);
    if(listenQueryFD < 0) {
        cerr <<  "- Error: socket()\n";
        return 1;
    }

    sockaddr_in queryServerAddr;
    bzero(&queryServerAddr, sizeof(queryServerAddr));
    
    queryServerAddr.sin_family = AF_INET;
    queryServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    queryServerAddr.sin_port = htons(queryPortNum);
    // Reuse socket if not closed properly
    int reuseQueryPort = 1;
    if (setsockopt(listenQueryFD, SOL_SOCKET, SO_REUSEADDR, &reuseQueryPort, sizeof(reuseQueryPort)) < 0) {
        cerr <<  "- Error: setsockopt()\n";
        return 1;
    }
    if(bind(listenQueryFD, (sockaddr*) &queryServerAddr, sizeof(queryServerAddr)) < 0) {
        cerr <<  "- Error: bind()\n";
        return 1;
    }
    if(listen(listenQueryFD, 10) < 0) {
        cerr <<  "- Error: listen()\n";
        return 1;
    }
    // fd_Set to monitor listenStatsFD and listenQueryFD with select
    fd_set fdSet;
    FD_ZERO(&fdSet);
    FD_SET(listenStatsFD, &fdSet);
    FD_SET(listenQueryFD, &fdSet);
    int maxFD = (listenStatsFD > listenQueryFD) ? listenStatsFD : listenQueryFD;

    cout << "-- SERVER STARTED --\n";
    // Start accepting connections
    while(true) {
        fd_set tempSet = fdSet;
        if(select(maxFD + 1, &tempSet, NULL, NULL, NULL) < 0) {
            cerr << "- Error: Select()\n";
            return 1;
        }
        // check if there is space in the buffer
        pthread_mutex_lock(&buffer_mutex);
        while(buffer->isFull())
            pthread_cond_wait(&buffer_pop_cond, &buffer_mutex);

        pthread_mutex_unlock(&buffer_mutex);

        if(FD_ISSET(listenStatsFD, &tempSet)) {
            pthread_mutex_lock(&clients_mutex);
            bool clientsPending = clients->length() != 0;
            pthread_mutex_unlock(&clients_mutex);
            // Check if clients are waiting response from already existing workers
            if(clientsPending) continue;

            sockaddr_in workerServerAddr;
            socklen_t socketLen = sizeof(workerServerAddr);
            // Accept connection from worker
            int statsFD = accept(listenStatsFD, (sockaddr*) &workerServerAddr, &socketLen);
            if(statsFD < 0) {
                cerr << "- Error: accept()\n";
                return 1;
            }
            // receive port to send queries and sumary stats from worker
            int workerPort = stoi(receiveMessage(statsFD));
            string summary = receiveMessage(statsFD);
            // Print summary stats of worker
            cout << summary;
            //  Create new socket to and connect to worker
            int workerFD = socket(AF_INET, SOCK_STREAM, 0);
            if(workerFD < 0) {
                cerr <<  "- Error: socket()\n";
                return 1;
            }

            workerServerAddr.sin_family = AF_INET;
            workerServerAddr.sin_port = htons(workerPort);
            // Connect to worker
            if(connect(workerFD, (sockaddr*) &workerServerAddr, sizeof(workerServerAddr)) < 0) {
                cerr << "- Error: connect()\n";
                return 1;
            }

            pthread_mutex_lock(&workers_mutex);
            workers->insert(statsFD);
            workersQueryFD_list->add(workerFD);
            pthread_mutex_unlock(&workers_mutex);

            pthread_mutex_lock(&buffer_mutex);
            buffer->push(statsFD);
            pthread_cond_signal(&buffer_push_cond);
            pthread_mutex_unlock(&buffer_mutex);
        }
        else if(FD_ISSET(listenQueryFD, &tempSet)) {
            int clientFD = accept(listenQueryFD, (sockaddr*) NULL, NULL);
            if(clientFD < 0) {
                cerr << "- Error: accept()\n";
                return 1;
            }
            // Add new fd in buffer
            pthread_mutex_lock(&buffer_mutex);
            buffer->push(clientFD);
            pthread_cond_signal(&buffer_push_cond);
            pthread_mutex_unlock(&buffer_mutex);
        }
    }
}
