#include <iostream>
#include <unistd.h>
#include "../HeaderFiles/ServerUtil.h"


ringBuffer::ringBuffer(int maxSize) {
    capacity = maxSize;
    buffer = new int[capacity];
    isCLient = new bool[capacity];
    first = 0;
    last = -1;
    size = 0;
}

ringBuffer::~ringBuffer() {
    delete[] buffer;
    delete[] isCLient;
}

int ringBuffer::length() {
    return size;
}

bool ringBuffer::isFull() {
    return size == capacity;
}

bool ringBuffer::isEmpty() {
    return size == 0;
}

bool ringBuffer::push(int fd, bool isCli) {
    if(isFull()) return false;
    
    size++;
    last = (last + 1) % capacity;
    buffer[last] = fd;
    isCLient[last] = isCli;

    return true;
}

bool ringBuffer::firstIsClient() {
    return !isEmpty() && isCLient[first];
}

int ringBuffer::pop() {
    if(isEmpty()) return -1;

    size--;
    int fd = buffer[first];
    first = (first + 1) % capacity;
    // Reset position
    if(isEmpty()) { 
        first = 0;
        last = -1;
    }
    
    return fd;
}


workerList::workerList() {
    head = NULL;
    size = 0;
}

workerList::~workerList() {
    empty(head);
}

void workerList::empty(node* n) {
	if(n == NULL) return;
	empty(n->next);
	delete n;
}

void workerList::insert(sockaddr_in workerAddr) {
    size++;

    node* temp = head;
    head = new node;
    head->addr = workerAddr;
    head->next = temp;
}

int workerList::length() { 
    return size;
}

int* workerList::connect() {
    int *fdArray = new int[size];
    int len = size;
    node **curr = &head;

    for(int i = 0; i < len; i++) {
        if((fdArray[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            cerr <<  "- Error: socket()\n";
            exit(EXIT_FAILURE);
        }
        // If connection failed remove worker from list else go to next
        if(::connect(fdArray[i], (sockaddr*) &((*curr)->addr), sizeof((*curr)->addr)) < 0) {
            size--;
            node* next = (*curr)->next;
            delete *curr;
            *curr = next;
            close(fdArray[i]);
            fdArray[i] = -1;
        }
        else
            curr = &((*curr)->next);
    }

    return fdArray;
}
