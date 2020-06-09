#pragma once
#include <string>
#include "../HeaderFiles/Util.h"


class ringBuffer {
private:
    intList *buffer;
    int capacity;

public:
    ringBuffer(int size);
    ~ringBuffer();
    bool isFull();
    bool isEmpty();
    bool push(int value);
    int pop();
};


class workerList {
private:
    struct node {
        int fd;
        strList *data;
        node *next;
    };

    node *head;
    int size;
    
public:
    workerList();
    void insert(int fd);
    void remove(int fd);
    int length();
    strList* getData(int fd);           // get list of data of node with fd
    void addData(int fd, string data);  // insert new string data in strList of node with fd
    bool member(int fd);                // Check if node with fd exists
    bool allHaveData();                 // Check if all node have data in their data strList
    string removeDatafromIndex(int target);     // Remove least recent data string from node at index target
    bool isLast(int fd);                // Check if node with fd is the least recent
};
