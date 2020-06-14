#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include "HeaderFiles/Util.h"
#include "HeaderFiles/ServerUtil.h"

using namespace std;


static volatile sig_atomic_t received_sigint = 0;

void handle_sigint(int sig) {
    received_sigint = 1;
}


workerList* workers = new workerList();
pthread_mutex_t worker_mutex = PTHREAD_MUTEX_INITIALIZER;

ringBuffer *buffer;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_push_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_pop_cond = PTHREAD_COND_INITIALIZER;


void handleWorker(int fd) {
    int port = stoi(receiveMessage(fd));
    string summary = receiveMessage(fd);
    // Get address of worker
    sockaddr_in workerAddr;
    socklen_t socketLen = sizeof(workerAddr);
    getpeername(fd, (sockaddr*) &workerAddr, &socketLen);
    workerAddr.sin_port = htons(port);
    // Insert worker in workers list
    pthread_mutex_lock(&worker_mutex);
    workers->check();
    workers->insert(fd, workerAddr);
    pthread_mutex_unlock(&worker_mutex);
    // Print summary
    cout << summary;
}


void handleClient(int fd) {
    pthread_mutex_lock(&worker_mutex);
    int numOfWorker = workers->length();
    int *workersFD = workers->connect();    // Connect to workers and get an fd for each connection
    pthread_mutex_unlock(&worker_mutex);
    // Setup select to wait for worker answer to wuery
    fd_set fdSet;
    FD_ZERO(&fdSet);
    int maxFD = -1;

    for(int w = 0; w < numOfWorker; w++) {
        FD_SET(workersFD[w], &fdSet);
        if(workersFD[w] > maxFD) maxFD = workersFD[w];
    }

    string query;
    while((query = receiveMessage(fd)) != END_READ) {   // While connection is open
        string res = "", queryName = "";
        // Get query name
        istringstream s(query);
        s >> queryName;

        bool isKnownQuery = 
            queryName == "/diseaseFrequency" ||
            queryName == "/numPatientAdmissions" ||
            queryName == "/searchPatientRecord" ||
            queryName == "/topk-AgeRanges" ||
            queryName == "/numPatientDischarges";
        // Process Query
        if(isKnownQuery) {
            for(int w = 0; w < numOfWorker; w++)
                sendMessage(workersFD[w], query);
            // Wait for all workers to send answer
            int freq = 0, workersResponded = 0;
            while(workersResponded < numOfWorker) {
                fd_set tempSet = fdSet;
                if(select(maxFD + 1, &tempSet, NULL, NULL, NULL) < 0) {
                    cerr << "- Error: Select()\n";
                    exit(EXIT_FAILURE);
                }
                // Check for answer in workers
                for(int w = 0; w < numOfWorker; w++)
                    if(FD_ISSET(workersFD[w], &tempSet)) {
                        string answer = receiveMessage(workersFD[w]);
                        if(answer != END_READ) {
                            if(queryName == "/diseaseFrequency")
                                freq += stoi(answer);
                            else
                                res += answer;
                        }

                        workersResponded++;
                        break;
                    }
            }
            
            if(queryName == "/diseaseFrequency") 
                res = to_string(freq) + "\n";
        }
        // If no answer found
        if(res == "") res = "--Not found\n\n";
        // Print result and send to client
        cout << query + "\n" + res + "\n";
        if(sendMessage(fd, res) <= 0) break;
    }
    // Close client connection
    close(fd);
    // Close connection with workers
    for(int w = 0; w < numOfWorker; w++)
        close(workersFD[w]);
    // Release allocated memory
    delete workersFD;
}


void* thread_function(void *arg) {
    while(true) {
        pthread_mutex_lock(&buffer_mutex);
        // Wait until there is an fd in buffer
        while(buffer->isEmpty()) {
            if(received_sigint) {
                pthread_mutex_unlock(&buffer_mutex);
                return NULL;
            }

            pthread_cond_wait(&buffer_push_cond, &buffer_mutex);
        } 
        // Get fd from buffer
        bool isClient = buffer->firstIsClient();
        int fd = buffer->pop();
        // Signal that an element removed from buffer
        pthread_cond_signal(&buffer_pop_cond);
        pthread_mutex_unlock(&buffer_mutex);
        // Handle fd
        isClient ? handleClient(fd) : handleWorker(fd);
    }

    return NULL;
}


int main(int argc, char* argv[]) {
    signal(SIGINT, handle_sigint);
    signal(SIGPIPE, SIG_IGN);       // Ignore signal from failing to write in socket

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

    // Setup worker connection listening socket
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
    // Setup client connection listening socket
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
    // Monitor listenStatsFD and listenQueryFD with select
    fd_set fdSet;
    FD_ZERO(&fdSet);
    FD_SET(listenStatsFD, &fdSet);
    FD_SET(listenQueryFD, &fdSet);
    int maxFD = (listenStatsFD > listenQueryFD) ? listenStatsFD : listenQueryFD;

    cout << "-- SERVER STARTED --\n";
    // Start accepting connections
    while(!received_sigint) {
        fd_set tempSet = fdSet;
        if(select(maxFD + 1, &tempSet, NULL, NULL, NULL) < 0) {
            if(received_sigint) break;

            cerr << "- Error: Select()\n";
            return 1;
        }

        pthread_mutex_lock(&buffer_mutex);
        // Wait until there is space in the buffer
        while(buffer->isFull() && !received_sigint)
            pthread_cond_wait(&buffer_pop_cond, &buffer_mutex);

        pthread_mutex_unlock(&buffer_mutex);
        // Terminate program
        if(received_sigint) break;

        if(FD_ISSET(listenStatsFD, &tempSet)) {
            int statsFD = accept(listenStatsFD, (sockaddr*) NULL, NULL);
            if(statsFD < 0) {
                cerr << "- Error: accept()\n";
                return 1;
            }
            // Push fd to buffer
            pthread_mutex_lock(&buffer_mutex);
            buffer->push(statsFD, false);
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
            buffer->push(clientFD, true);
            pthread_cond_signal(&buffer_push_cond);     // Signal that an element was added to buffer
            pthread_mutex_unlock(&buffer_mutex);

            // cout << "- Client with file descriptor " + to_string(clientFD) + " connected\n\n";
        }
    }
    // Signal threads to stop waiting for new fd in buffer
    pthread_cond_broadcast(&buffer_push_cond);
    // Wait for all threads to complete
    for(int i = 0; i < numThreads; i++)
        pthread_join(thread_pool[i], NULL);
    // Clear allocated memory
    delete workers;
    delete buffer;

    return 0;
}
