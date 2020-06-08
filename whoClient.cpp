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
#include <pthread.h>

#include "HeaderFiles/Util.h"

using namespace std;


string serverIP;
int serverPort;
strList *query_list = new strList();

pthread_mutex_t queryList_mutex = PTHREAD_MUTEX_INITIALIZER;


void* thread_function(void *arg) {
    while(true) {
        pthread_mutex_lock(&queryList_mutex);
        // Check if query list is empty
        if(query_list->length() == 0) {
            pthread_mutex_unlock(&queryList_mutex);
            return NULL;
        }
        // Get query from list
        string query = query_list->dequeue();

        pthread_mutex_unlock(&queryList_mutex);

        // Connect to server
        int clientFD = socket(AF_INET, SOCK_STREAM, 0);
        if(clientFD < 0) {
            cerr << "- Error: Socket()\n";
            return NULL;
        }

        sockaddr_in serverAddr;
        bzero(&serverAddr, sizeof(serverAddr));

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);

        if(inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            cerr << "- Error: inet_pton()\n";
            return NULL;
        }
        if(connect(clientFD, (sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "- Error: connect()\n";
            return NULL;
        }
        // Send query
        sendMessage(clientFD, query);
        // Receive result from server
        string queryAnswer = receiveMessage(clientFD);
        // Print query and result
        cout << query + "\n" + queryAnswer + "\n";

        close(clientFD);
    }
}


int main(int argc, char* argv[]) {
    string queryFile;
    int numThreads;

    if(argc < 9) {
        cerr << "- Error: Not Enough Parameters\n";
        return 1;
    }

    for(int i = 1; i < argc; i += 2) {
        string temp = argv[i];

        if(!temp.compare("-q"))
            queryFile = argv[i+1];
        else if(!temp.compare("-sp"))
            serverPort = atoi(argv[i+1]);
        else if(!temp.compare("-w"))
            numThreads = atoi(argv[i+1]);
        else if(!temp.compare("-sip"))
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

    string query;
    ifstream ifs(queryFile);
    // Read queries from file
    while(getline(ifs, query))
        if(query != "")
            query_list->add(query);
    // Create at most query length threads
    if(numThreads > query_list->length())
        numThreads = query_list->length();
    // Init threads
    pthread_t thread_pool[numThreads];
    for(int i = 0; i < numThreads; i++)
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    // Wait for threads to complete
    for (int i = 0; i < numThreads; i++)
       pthread_join(thread_pool[i], NULL);

    cout.flush();
    return 0;
}
