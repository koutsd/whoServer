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

#include "HeaderFiles/Hash_Table.h"
#include "HeaderFiles/Util.h"

using namespace std;


struct Dir {
    string name;
    strList *files;
};


static volatile sig_atomic_t received_sigint = 0;
static volatile sig_atomic_t received_sigusr1 = 0;

void handle_sigint(int sig) {
    received_sigint = 1;
}

void handle_sigusr1(int sig) {
    received_sigusr1 = 1;
}


int main(int argc, char* argv[]) {
    signal(SIGINT, handle_sigint);
    signal(SIGQUIT, handle_sigint);
    signal(SIGUSR1, handle_sigusr1);

    string fifo_file = "/tmp/myfifo";

    if(argc < 3) return 1;

    int id = atoi(argv[1]);
    int bufferSize = atoi(argv[2]);

    if(bufferSize < 2) {
        cerr <<  "- Error: Buffer size must be at least 2 Bytes\n";
        return 1;
    }

    string readFIFO = fifo_file + to_string(2*id + 1);
    string writeFIFO = fifo_file + to_string(2*id);
    // Init FIFOs
    mkfifo(readFIFO.c_str(), 0666);
    mkfifo(writeFIFO.c_str(), 0666);

    int readPipe = open(readFIFO.c_str(), O_RDONLY);
    int writePipe = open(writeFIFO.c_str(), O_WRONLY);
    // Server IP
    string serverIP = receiveMessage(readPipe, bufferSize);
    // Server Port
    int serverPort = stoi(receiveMessage(readPipe, bufferSize));
    // Connect to server
    int statsFD = socket(AF_INET, SOCK_STREAM, 0);
    if(statsFD < 0) {
        cerr << "- Error: Socket()\n";
        return 1;
    }

    sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    if(inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "- Error: inet_pton()\n";
        return 1;
    }
    if(connect(statsFD, (sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "- Error: connect()\n";
        return 1;
    }
    // Read num of directories for workers
    int numOfDirs = stoi(receiveMessage(readPipe, bufferSize));
    if(numOfDirs <= 0) {
        cerr << "- Error: Worker must have at least 1 Directory\n";
        return 1;
    }
    // Read paths of worker's direcotries
    Dir directories[numOfDirs];
    for(int i = 0; i < numOfDirs; i++)
        directories[i].name = receiveMessage(readPipe, bufferSize);
    // Data structs to store patient Records
    Hash_Table *entry_table[numOfDirs];
    AVL_Tree *exit_tree[numOfDirs];

    string patientID, action, fName, lName, disease, country, date, line, summary = "";
    int age, entryDate, exitDate;
    // Read records from all country Directories
    for(int i = 0; i < numOfDirs; i++) {
        entry_table[i] = new Hash_Table(10, "Disease");
        exit_tree[i] = new AVL_Tree();
        directories[i].files = new strList();

        country = directories[i].name.substr(directories[i].name.find_last_of("/") + 1);

        DIR *dir;
        struct dirent *entry;

        if (!(dir = opendir(directories[i].name.c_str()))) {
            cerr << "- Error: Unable to open directory\n";
            return 1;
        }
        // Temporarily store EXIT records to insert later
        PatientList *tempList = new PatientList();
        // Read files from country Dir
        while(entry = readdir(dir)) {
            if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                continue;
            // Date file
            date = entry->d_name;
            directories[i].files->add(date);

            ifstream ifs(directories[i].name + "/" + date);

            while(getline(ifs, line)) {
                istringstream iss(line);

                if(!(iss >> patientID >> action >> fName >> lName >> disease >> age)) {
                    cerr << "- Error: Unable to read Record\n";
                    return 1;
                }

                PatientRecord *temp = entry_table[i]->find(patientID);
                // Insert Patient record into hash tables
                if(action == "ENTER") {
                    if(temp != NULL)
                        cerr << "ERROR\n";
                    else {
                        PatientRecord *p = new PatientRecord(patientID, fName, lName, disease, country, age, dateToInt(date), -1);
                        entry_table[i]->insert(p);
                    }
                }
                else if(action == "EXIT") {
                    if(temp == NULL) {                 // Keep in tempList to check later
                        PatientRecord *p = new PatientRecord(patientID, fName, lName, disease, country, age, -1, dateToInt(date));
                        tempList->insert(p);
                    }
                    else if(temp->exitDate == -1 && temp->entry() <= dateToInt(date)) {   // Update exit date and insert to exit_tree
                        temp->exitDate = dateToInt(date);
                        exit_tree[i]->insert_2(temp);
                    }
                    else                            // Patient with this id already has exit date
                        cerr << "ERROR\n";
                }
            }

            ifs.close();
            summary += date + "\n" + country + "\n" + entry_table[i]->summary(dateToInt(date));
        }

        closedir(dir);
        // Recheck records in tempList
        PatientRecord *temp = NULL;
        int index = 0;
        while((temp = tempList->pop()) != NULL) {
            PatientRecord *p = entry_table[i]->find(temp->id());
            // Record patient EXIT
            if(p != NULL && p->exitDate == -1 && p->entry() <= temp->exitDate) {
                p->exitDate = temp->exitDate;
                exit_tree[i]->insert_2(p);
            }
            else
                cerr << "ERROR\n";

            delete temp;
        }

        delete tempList;
    }
    // Init new socket for server to connect and send queries
    int listenQueryFD = socket(AF_INET, SOCK_STREAM, 0);
    if(listenQueryFD < 0) {
        cerr << "- Error: Socket()\n";
        return 1;
    }

    sockaddr_in queryServerAddr;
    bzero(&queryServerAddr, sizeof(queryServerAddr));

    queryServerAddr.sin_family = AF_INET;
    queryServerAddr.sin_port = 0;
    queryServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listenQueryFD, (struct sockaddr *) &queryServerAddr, sizeof(queryServerAddr)) < 0){
        cerr << "- Error: bind()\n";
        return 1;
    }
    // Get port number
    socklen_t socketLen = sizeof(queryServerAddr);
    if (getsockname(listenQueryFD, (struct sockaddr *) &queryServerAddr, &socketLen) < 0){
        cerr << "- Error: getsockname()\n";
        return 1;
    }
    if(listen(listenQueryFD, 10) < 0) {
        cerr <<  "- Error: listen()\n";
        return 1;
    }
    // Send port number and summary stats to server
    sendMessage(statsFD, to_string(ntohs(queryServerAddr.sin_port)));
    sendMessage(statsFD, summary);
    // Accept connection from server
    int queryFD = accept(listenQueryFD, (sockaddr*) NULL, NULL);
    if(queryFD < 0) {
        cerr << "- Error: accept()\n";
        return 1;
    }

    fd_set fdSet;
    FD_ZERO(&fdSet);
    FD_SET(queryFD, &fdSet);
    // Init statistics for log_file
    int success = 0;
    int fail = 0;
    // Wait and process queries from parent
    string query;
    while(!received_sigint) {
        string param1 = "", param2 = "", param3 = "", line = "";
        int date1, date2, temp;
        // Wait for parent to send query
        fd_set tempSet = fdSet;
        while(!received_sigint && !received_sigusr1) {
            if(select(queryFD + 1, &tempSet, NULL, NULL, NULL) < 0)
                if(received_sigint || received_sigusr1)     // Select failed due to signal
                    break;
                else {
                    cerr << "- Error: Select()\n";
                    return 1;
                }

            if(FD_ISSET(queryFD, &tempSet)) {
                line = receiveMessage(queryFD);
                cout << "- Worker " + to_string(id) + " received query: " + line + "\n";
                break;
            }
        }
        // Handle SIGINT/SIGQUIT --> Exit application
        if(received_sigint)
            break;
        // Handle SIGUSR1 --> insert new records and inform parent
        if(received_sigusr1) {
            cerr << endl;
            received_sigusr1 = 0;
            summary = "";
            // Search directories for new files
            for(int i = 0; i < numOfDirs; i++) {
                string country = directories[i].name.substr(directories[i].name.find_last_of("/") + 1);

                DIR *dir;
                struct dirent *entry;

                if (!(dir = opendir(directories[i].name.c_str()))) {
                    cerr << "- Error: Unable to open directory" << endl;
                    return 1;
                }
                // Temporarily store EXIT records to insert later
                PatientList *tempList = new PatientList();
                // Read files from country Dir
                while(entry = readdir(dir)) {
                    if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                        continue;
                    // Date file
                    date = entry->d_name;
                    if(directories[i].files->member(date))  // Find new date file
                        continue;

                    directories[i].files->add(date);

                    ifstream ifs(directories[i].name + "/" + date);

                    while(getline(ifs, line)) {
                        istringstream iss(line);

                        if(!(iss >> patientID >> action >> fName >> lName >> disease >> age)) {
                            cerr << "- Error: Unable to read Record" << endl;
                            return 1;
                        }

                        PatientRecord *temp = entry_table[i]->find(patientID);
                        // Insert Patient record into hash tables
                        if(action == "ENTER") {
                            if(temp != NULL)
                                cerr << "ERROR\n";
                            else {
                                PatientRecord *p = new PatientRecord(patientID, fName, lName, disease, country, age, dateToInt(date), -1);
                                entry_table[i]->insert(p);
                            }
                        }
                        else if(action == "EXIT") {
                            // Edit record exit Date
                            if(temp == NULL) {                 // Keep in tempList to check later
                                PatientRecord *p = new PatientRecord(patientID, fName, lName, disease, country, age, -1, dateToInt(date));
                                tempList->insert(p);
                            }
                            else if(temp->exitDate == -1 && temp->entry() <= dateToInt(date)) {   // Update exit date and insert to exit_tree
                                temp->exitDate = dateToInt(date);
                                exit_tree[i]->insert_2(temp);
                            }
                            else                            // Patient with this id already has exit date
                                cerr << "ERROR\n";
                        }
                    }

                    ifs.close();
                    summary += date + "\n" + country + "\n" + entry_table[i]->summary(dateToInt(date));
                }

                closedir(dir);
                // Recheck records in tempList
                PatientRecord *temp = NULL;
                int index = 0;
                while((temp = tempList->pop()) != NULL) {
                    PatientRecord *p = entry_table[i]->find(temp->id());
                    // Record patient EXIT
                    if(p != NULL && p->exitDate == -1 && p->entry() <= temp->exitDate) {
                        p->exitDate = temp->exitDate;
                        exit_tree[i]->insert_2(p);
                    }
                    else
                        cerr << "ERROR\n";

                    delete temp;
                }

                delete tempList;
            }

            kill(getppid(), SIGUSR1);
            sendMessage(statsFD, summary);
            continue;
        }

        istringstream s(line);
        s >> query;

        if(query == "/diseaseFrequency") {
            s >> disease >> param1 >> param2 >> param3;

            if(!isDate(param1) || !isDate(param2)) {
                fail++;
                sendMessage(statsFD, "-1");
                continue;
            }

            date1 = dateToInt(param1);
            date2 = dateToInt(param2);

            if(date1 > date2) {
                int temp = date1;
                date1 = date2;
                date2 = temp;
            }

            int frequency = 0;
            for(int i = 0; i < numOfDirs; i++) {
                string country = directories[i].name.substr(directories[i].name.find_last_of("/") + 1);
                if(param3 == "" || param3 == country) {
                    frequency += entry_table[i]->frequency(disease, date1, date2);
                    if(param3 == country) break;
                }
            }

            frequency == 0 ? fail++ : success++;
            sendMessage(statsFD, to_string(frequency));
        }
        else if(query == "/numPatientAdmissions") {
            s >> disease >> param1 >> param2 >> param3;

            if(!isDate(param1) || !isDate(param2)) {
                fail++;
                sendMessage(statsFD, "");
                continue;
            }

            date1 = dateToInt(param1);
            date2 = dateToInt(param2);

            if(date1 > date2) {
                int temp = date1;
                date1 = date2;
                date2 = temp;
            }

            string admissions = "";
            for(int i = 0; i < numOfDirs; i++) {
                string country = directories[i].name.substr(directories[i].name.find_last_of("/") + 1);
                if(param3 == "" || param3 == country) {
                    admissions += country + " " + to_string(entry_table[i]->frequency(disease, date1, date2)) + "\n";
                    if(param3 == country) break;
                }
            }

            admissions == "" ? fail++ : success++;
            sendMessage(statsFD, admissions);
        }
        else if(query == "/searchPatientRecord") {
            if(line == query) {
                fail++;
                sendMessage(statsFD, "");
                continue;
            }

            s >> patientID;

            string res = "";
            PatientRecord *p;
            for(int i = 0; i < numOfDirs; i++)
                if((p = entry_table[i]->find(patientID)) != NULL) {
                    res = p->id() + " " 
                        + p->firstName() + " " 
                        + p->lastName() + " " 
                        + p->disease()+ " " 
                        + to_string(p->getAge()) + " " 
                        + intToDate(p->entry()) + " " 
                        + intToDate(p->exitDate) + "\n";

                    break;
                }

            res == "" ? fail++ : success++;
            sendMessage(statsFD, res);
        }
        else if(query == "/topk-AgeRanges") {
            int k;
            s >> k >> param3 >> disease >> param1 >> param2;

            if(!isDate(param1) || !isDate(param2)) {
                fail++;
                sendMessage(statsFD, "");
                continue;
            }

            date1 = dateToInt(param1);
            date2 = dateToInt(param2);

            if(date1 > date2) {
                int temp = date1;
                date1 = date2;
                date2 = temp;
            }

            string res = "";
            for(int i = 0; i < numOfDirs; i++) {
                string country = directories[i].name.substr(directories[i].name.find_last_of("/") + 1);
                if(param3 == country) {
                    res = entry_table[i]->topK_AgeRanges(k, disease, date1, date2);
                    break;
                }
            }

            res == "" ? fail++ : success++;
            sendMessage(statsFD, res);
        }
        else if(query == "/numPatientDischarges") {
            s >> disease >> param1 >> param2 >> param3;
            
            if(!isDate(param1) || !isDate(param2)) {
                fail++;
                sendMessage(statsFD, "");
                continue;
            }

            date1 = dateToInt(param1);
            date2 = dateToInt(param2);

            if(date1 > date2) {
                int temp = date1;
                date1 = date2;
                date2 = temp;
            }

            PatientRecord *p = new PatientRecord("", "", "", disease, "", -1, -1, -1);

            string discharges = "";
            for(int i = 0; i < numOfDirs; i++) {
                string country = directories[i].name.substr(directories[i].name.find_last_of("/") + 1);
                if(param3 == "" || param3 == country) {
                    discharges += country + " " + to_string(exit_tree[i]->count(date1, date2, p)) + "\n";
                    if(param3 == country) break;
                }
            }

            delete p;
            discharges == "" ? fail++ : success++;
            sendMessage(statsFD, discharges);
        }
        else
            sendMessage(statsFD, "");
    }
    // Create log File
    ofstream logfile("log/log_file." + to_string(getpid()));

    for(int i = 0; i < numOfDirs; i++)
        logfile <<  directories[i].name.substr(directories[i].name.find_last_of("/") + 1) + "\n";

    logfile << "TOTAL " + to_string(success + fail) + "\n" 
            + "SUCCESS " + to_string(success) + "\n"
            + "FAIL " + to_string(fail) + "\n";

    logfile.close();
    // Close pipes
    close(readPipe);
    close(writePipe);
    close(statsFD);
    // Free allocated memory
    for(int i = 0; i < numOfDirs; i++) {
        entry_table[i]->deleteRecords();
        delete entry_table[i];
        delete exit_tree[i];
        delete directories[i].files;
    }

    return 0;
}
