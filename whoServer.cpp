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
    int sendFD = socket(AF_INET, SOCK_STREAM, 0);

    pthread_mutex_lock(&clients_mutex);
    // Wait until all client finished receiving data from connected workers
    while(clients->length() != 0)
        pthread_cond_wait(&clients_cond, &clients_mutex);
    // Connect to worker
    pthread_mutex_lock(&workers_mutex);
    workers->connect(fd, sendFD);
    pthread_mutex_unlock(&workers_mutex);

    pthread_mutex_unlock(&clients_mutex);
    // while worker active keep fd in thread
    string stats;
    while((stats = receiveMessage(fd)) != END_READ) {    // END_READ --> indicates closed connection
        // Insert query response in data struct
        pthread_mutex_lock(&workers_mutex);
        workers->addData(fd, stats);
        // Signal that worker sent new data
        pthread_cond_signal(&workers_cond);     
        pthread_mutex_unlock(&workers_mutex);
    }
    // Remove dead worker from list
    pthread_mutex_lock(&workers_mutex);
    workers->remove(fd);
    pthread_mutex_unlock(&workers_mutex);
}


void handleClient(int fd) {
    string query = receiveMessage(fd);
    if(query == END_READ) return;    // Error reading fd

    pthread_mutex_lock(&clients_mutex);
    // Send query to workers
    pthread_mutex_lock(&workers_mutex);
    workers->sendMessage(query);
    pthread_mutex_unlock(&workers_mutex);
    // Add client fd to queue
    clients->add(fd);
    // Wait for client's turn receive results
    while(fd != clients->get(clients->length() - 1))
        pthread_cond_wait(&clients_cond, &clients_mutex);
    // Client no longer in queue
    clients->dequeue();

    pthread_mutex_lock(&workers_mutex);
    // Compose query answer
    string res;
    while((res = workers->getStats(query)) == END_READ)     // Wait untill all worker have answer to query
        pthread_cond_wait(&workers_cond, &workers_mutex);

    pthread_mutex_unlock(&workers_mutex);
    // Signal clients change
    pthread_cond_signal(&clients_cond);
    pthread_mutex_unlock(&clients_mutex);
    // Print and send query answer
    if(sendMessage(fd, res) <= 0) return;
    cout << query + "\n" + res + "\n";
}


void* thread_function(void *arg) {
    while(true) {
        pthread_mutex_lock(&buffer_mutex);
        // Wait until there is an fd in buffer
        while(buffer->isEmpty())
            pthread_cond_wait(&buffer_push_cond, &buffer_mutex);
        // Get fd from buffer
        int fd = buffer->pop();
        // Signal that an element removed from buffer
        pthread_cond_signal(&buffer_pop_cond);
        pthread_mutex_unlock(&buffer_mutex);
        // Check if fd belongs to worker connection
        pthread_mutex_lock(&workers_mutex);
        bool isWorker = workers->member(fd);
        pthread_mutex_unlock(&workers_mutex);
        // Handle fd
        isWorker ? handleWorker(fd) : handleClient(fd);
        
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

        if(temp == "-q")
            queryPortNum = atoi(argv[i+1]);
        else if(temp == "-s")
            statisticsPortNum = atoi(argv[i+1]);
        else if(temp == "-w")
            numThreads = atoi(argv[i+1]);
        else if(temp == "-b")
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
    // Initialize ring buffer
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
    if(listen(listenStatsFD, 512) < 0) {
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
    if(listen(listenQueryFD, 512) < 0) {
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

        pthread_mutex_lock(&buffer_mutex);
        // Wait until there is space in the buffer
        while(buffer->isFull())
            pthread_cond_wait(&buffer_pop_cond, &buffer_mutex);

        pthread_mutex_unlock(&buffer_mutex);

        if(FD_ISSET(listenStatsFD, &tempSet)) {
            sockaddr_in workerAddr;
            socklen_t socketLen = sizeof(workerAddr);
            // Accept connection from worker
            int statsFD = accept(listenStatsFD, (sockaddr*) &workerAddr, &socketLen);
            if(statsFD < 0) {
                cerr << "- Error: accept()\n";
                return 1;
            }
            // receive port to send queries and sumary stats from worker
            int workerPort = stoi(receiveMessage(statsFD));
            workerAddr.sin_port = htons(workerPort);
            // Print summary stats of worker
            string summary = receiveMessage(statsFD);
            cout << summary;
            // Add new worker
            pthread_mutex_lock(&workers_mutex);
            workers->insert(statsFD, workerAddr);
            pthread_mutex_unlock(&workers_mutex);
            // Push fd to buffer
            pthread_mutex_lock(&buffer_mutex);
            buffer->push(statsFD);
            pthread_cond_signal(&buffer_push_cond);     // Signal that an element was added to buffer
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
            pthread_cond_signal(&buffer_push_cond);     // Signal that an element was added to buffer
            pthread_mutex_unlock(&buffer_mutex);
        }
    }

    delete workers;
    delete clients;
    delete buffer;
}
