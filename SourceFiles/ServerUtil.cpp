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
    return size > 0 && isCLient[first];
}

int ringBuffer::pop() {
    if(isEmpty()) return -1;

    size--;
    int fd = buffer[first];
    first = (first + 1) % capacity;

    if(size == 0) {
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
	if (n == NULL)
	    return;

	empty(n->next);
    close(n->fd);
	delete n;
}

void workerList::insert(int fd, sockaddr_in workerAddr) {
    size++;

    node* temp = head;
    head = new node;
    head->fd = fd;
    head->addr = workerAddr;
    head->next = temp;
}

void workerList::check() {
    node **curr = &head;
    int checked = 0;
    int tempSize = size;

    while(checked++ < tempSize) {
        if(write((*curr)->fd, "", 1) <= 0) {
            size--;
            node* temp = (*curr)->next;
            close((*curr)->fd);
            delete *curr;
            *curr = temp;
            cout << "err\n";
        }
        else
            curr = &((*curr)->next);        
    }
}

int workerList::length() { 
    return size;
}

int* workerList::connect() {
    int *fdArray = new int[size];
    node *curr = head;
    for(int i = 0; i < size; i++) {
        if((fdArray[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            cerr <<  "- Error: socket()\n";
            exit(EXIT_FAILURE);
        }

        ::connect(fdArray[i], (sockaddr*) &(curr->addr), sizeof(curr->addr));
        curr = curr->next;
    }

    return fdArray;
}
