#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <dirent.h>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>
#include "HeaderFiles/Util.h"

enum {READ = 0, WRITE = 1};

using namespace std;

static volatile sig_atomic_t signals_received = 0;
static volatile sig_atomic_t received_sigint = 0;
static volatile sig_atomic_t received_sigchld = 0;
static volatile sig_atomic_t received_sigusr1 = 0;

void handle_sigint(int sig) {
    signals_received++;
    received_sigint = 1;
}

void handle_sigchld(int sig) {
    signals_received++;
    received_sigchld++;
}

void handle_sigusr1(int sig) {
    signals_received++;
    received_sigusr1++;
}


int main(int argc, char* argv[]) {
    signal(SIGCHLD, handle_sigchld);
    signal(SIGINT, handle_sigint);
    signal(SIGQUIT, handle_sigint);
    signal(SIGUSR1, handle_sigusr1);

    string fifo_file = "/tmp/myfifo";

    int numWorkers, serverPort, bufferSize;
    string input_dir, serverIP;

    if(argc < 9) {
        cerr << "- Error: Not Enough Parameters" << endl;
        return 1;
    }

    for(int i = 1; i < argc; i += 2) {
        string temp = argv[i];

        if(temp == "-i")
            input_dir = argv[i+1];
        else if(temp == "-w")
            numWorkers = atoi(argv[i+1]);
        else if(temp == "-s")
            serverIP = argv[i+1];
        else if(temp == "-p")
            serverPort = atoi(argv[i+1]);
        else if(temp == "-b")
            bufferSize = atoi(argv[i+1]);
        else {
            cerr << "- Error: Invalid Parameter: " << temp << endl;
            return 1;
        }
    }

    if(numWorkers <= 0) {
        cerr <<  "- Error: At least 1 Worker needed" << endl;
        return 1;
    }
    if(bufferSize < 2) {
        cerr <<  "- Error: Buffer size must be at least 2 Bytes" << endl;
        return 1;
    }

    DIR *dir;
    struct dirent *entry;
    strList *directories = new strList();

    if (!(dir = opendir(input_dir.c_str()))) {
        cerr << "- Error: Unable to open directory" << endl;
        return 1;
    }
    // Get all country directory names
    while(entry = readdir(dir))
        if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            directories->add(entry->d_name);

    closedir(dir);
    // Dont create unnecessary workers
    if(numWorkers > directories->length()) 
        numWorkers = directories->length();

    int workerPID[numWorkers];
    // Init workers
    for(int w = 0; w < numWorkers; w++) {
        int pid = fork();
        if(pid == 0) {
            execl("./worker", "worker", to_string(w).c_str(), to_string(bufferSize).c_str(), NULL);
            exit(0);
        }
        else if(pid == -1) {
            cerr << "- Error: Fork failed" << endl;
            return 1;
        }
        else {
            workerPID[w] = pid;
        }
    }

    int dirsLeft = directories->length();   // Num of directories not assigned to workers yet
    int pipes[numWorkers][2];
    strList *dirsPerWorker[numWorkers];     // Hold directories of each workers
    string summary = "";

    fd_set fdSet;
    FD_ZERO(&fdSet);
    int maxfd = 0;
    // Assign directories to workers
    for(int w = 0; w < numWorkers; w++) {
        dirsPerWorker[w] = new strList();

        string readFIFO = fifo_file + to_string(2*w);
        string writeFIFO = fifo_file + to_string(2*w + 1);
        // Create FIFOs
        mkfifo(writeFIFO.c_str(), 0666);
        mkfifo(readFIFO.c_str(), 0666);

        pipes[w][WRITE] = open(writeFIFO.c_str(), O_WRONLY);
        pipes[w][READ] = open(readFIFO.c_str(), O_RDONLY);

        if(pipes[w][READ] > maxfd)
            maxfd = pipes[w][READ];

        FD_SET(pipes[w][READ], &fdSet);

        int numOfDirsForWorker = dirsLeft / (numWorkers - w);       // Evenly distribute directories to workers
        sendMessage(pipes[w][WRITE], serverIP, bufferSize);
        sendMessage(pipes[w][WRITE], to_string(serverPort), bufferSize);
        sendMessage(pipes[w][WRITE], to_string(numOfDirsForWorker), bufferSize);
        // Send workers their direcotries
        for(int i = 0; i < numOfDirsForWorker; i++) {
            string country = directories->get(directories->length() - dirsLeft--);
            dirsPerWorker[w]->add(country);

            string subDir = input_dir + "/" + country;
            sendMessage(pipes[w][WRITE], subDir, bufferSize);
        }
    }

    delete directories;

    while(!received_sigint) {
        // Handle SIGCHLD --> init new worker
        while(received_sigchld) {
            for(int w = 0; w < numWorkers; w++) {
                int status;
                if(!waitpid(workerPID[w], &status, WNOHANG))    // Find terminated worker
                    continue;

                cout << "- Worker with PID " << workerPID[w] << " stopped" << endl << "- Initializing new Worker..." << endl;
                // Create new Worker
                int pid = fork();
                if(pid == 0) {
                    execl("./worker", "worker", to_string(w).c_str(), to_string(bufferSize).c_str(), NULL);
                    exit(0);
                }
                else if(pid == -1) {
                    cerr << "- Error: Fork failed" << endl;
                    return 1;
                }
                else {
                    workerPID[w] = pid;
                }

                string readFIFO = fifo_file + to_string(2*w);
                string writeFIFO = fifo_file + to_string(2*w + 1);       
                // Close old FIFOs
                close(pipes[w][READ]);
                close(pipes[w][WRITE]);
                // Remove from fd_set
                FD_CLR(pipes[w][READ], &fdSet);
                // Reinit FIFOs
                pipes[w][READ] = open(readFIFO.c_str(), O_RDONLY);
                pipes[w][WRITE] = open(writeFIFO.c_str(), O_WRONLY);

                FD_SET(pipes[w][READ], &fdSet);

                if(maxfd < pipes[w][READ])
                    maxfd = pipes[w][READ];
                
                sendMessage(pipes[w][WRITE], serverIP, bufferSize);
                sendMessage(pipes[w][WRITE], to_string(serverPort), bufferSize);
                sendMessage(pipes[w][WRITE], to_string(dirsPerWorker[w]->length()), bufferSize);

                for(int i = 0; i < dirsPerWorker[w]->length(); i++) {
                    string country = dirsPerWorker[w]->get(i);
                    string subDir = input_dir + "/" + country;
                    sendMessage(pipes[w][WRITE], subDir, bufferSize);
                }
                // Receive summary stats of new worker
                signals_received--;
                if(--received_sigchld == 0)
                    break;
            }
        }
        // Handle SIGUSR1 --> Wait for summary stats from workers that sent the sigusr1
        while(received_sigusr1) {
            fd_set tempSet = fdSet;

            if(select(maxfd + 1, &tempSet, NULL, NULL, NULL) < 0)
                if(signals_received)
                    continue;
                else {
                    cerr << "- Error: Select()\n";
                    return 1;
                }
                    

            for(int w = 0; w < numWorkers; w++)
                if(FD_ISSET(pipes[w][READ], &tempSet)) {
                    cout << receiveMessage(pipes[w][READ]) << "- Worker updated patient Records" << endl << endl;      // summary
                    received_sigusr1--;
                    signals_received--;
                    FD_CLR(pipes[w][READ], &tempSet);
                    break;
                }
        }
    }
    // Exit
    // Terminate workers
    for(int w = 0; w < numWorkers; w++)
        kill(workerPID[w], SIGKILL);

    int status = 0;
    while (wait(&status) > 0);
    // Close pipes and free allocated memory
    for(int w = 0; w < numWorkers; w++) {
        string readFIFO = fifo_file + to_string(2*w);
        string writeFIFO = fifo_file + to_string(2*w + 1);

        close(pipes[w][READ]);
        close(pipes[w][WRITE]);

        unlink(readFIFO.c_str());
        unlink(writeFIFO.c_str());

        delete dirsPerWorker[w];
    }

    cout << endl;
    return 0;
}
