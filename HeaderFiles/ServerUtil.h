#pragma once
#include <string>
#include <arpa/inet.h>
#include "../HeaderFiles/Util.h"


class ringBuffer {
private:
    bool *isCLient;
    int *buffer;
    int first, last;
    int capacity, size;

public:
    ringBuffer(int size);
    ~ringBuffer();
    int length();
    bool isFull();
    bool isEmpty();
    bool push(int fd, bool isCLi);
    bool firstIsClient();
    int pop();
};


class workerList {
private:
    struct node {
        int fd;    // StatsFD is used as a unique identifier of node
        sockaddr_in addr;
        node *next;
    };

    node *head;
    int size;
    
    void empty(node *n);
    
public:
    workerList();
    ~workerList();
    void insert(int fd, sockaddr_in workerAddr);
    void check_connection();
    int* connect();
    int length();
};
