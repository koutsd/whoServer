#pragma once
#include <string>
#include <sys/stat.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../HeaderFiles/Util.h"


class ringBuffer {
private:
    intList *buffer;
    int capacity;

public:
    ringBuffer(int size);
    ~ringBuffer();
    int length();
    bool isFull();
    bool isEmpty();
    bool push(int value);
    int pop();
};


class workerList {
private:
    struct node {
        int statsFD, sendFD;
        sockaddr_in addr;
        strList *data;
        node *next;
    };

    node *head;
    int size;
    
    void empty(node *n);
    
public:
    workerList();
    ~workerList();
    void insert(int statsFD, sockaddr_in workerAddr);
    void remove(int fd);
    int connect(int fd, int sendSocket);
    void sendMessage(string msg);
    int length();
    void addData(int fd, string data);  // insert new string data in strList of node with fd
    bool member(int fd);                // Check if node with fd exists
    bool allHaveData();                 // Check if all node have data in their data strList
    string getStats(string query);
};
