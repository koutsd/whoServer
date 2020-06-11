#include <iostream>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include "../HeaderFiles/ServerUtil.h"


ringBuffer::ringBuffer(int size) {
    buffer = new intList();
    capacity = size;
}

ringBuffer::~ringBuffer() {
    delete buffer;
}

int ringBuffer::length() {
    return buffer->length();
}

bool ringBuffer::isFull() {
    return buffer->length() == capacity;
}

bool ringBuffer::isEmpty() {
    return buffer->length() == 0;
}

bool ringBuffer::push(int value) {
    if(isFull())
        return false;

    buffer->add(value);
    return true;
}

int ringBuffer::pop() {
    return buffer->dequeue();
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
    close(n->statsFD);
    close(n->sendFD);
    delete n->data;
	delete n;
}

void workerList::insert(int statsFD, sockaddr_in workerAddr) {
    size++;

    node* temp = head;
    head = new node;
    head->statsFD = statsFD;
    head->sendFD = -1;
    head->data = new strList();
    head->addr = workerAddr;
    head->next = temp;
}

void workerList::remove(int fd) {
    for(node **curr = &head; (*curr) != NULL; curr = &((*curr)->next))
        if(fd == (*curr)->statsFD) {
            size--;
            node* temp = (*curr)->next;
            close((*curr)->statsFD);
            close((*curr)->sendFD);
            delete (*curr)->data;
            delete *curr;
            *curr = temp;

            return;
        }
}

int workerList::length() { 
    return size;
}

int workerList::connect(int fd, int sendSocket) { 
    for(node *curr = head; curr != NULL; curr = curr->next)
        if(fd == curr->statsFD && curr->sendFD == -1) {
            int ret = ::connect(sendSocket, (sockaddr*) &(curr->addr), sizeof(curr->addr));
            if(ret == 0) 
                curr->sendFD = sendSocket;

            return ret;
        }
    
    return -1;
}

void workerList::addData(int fd, string data) { 
    for(node *curr = head; curr != NULL; curr = curr->next)
        if(fd == curr->statsFD) {
            curr->data->add(data);
            return;
        }
}

bool workerList::member(int fd) {
    for(node *curr = head; curr != NULL; curr = curr->next)
        if(fd == curr->statsFD)
            return true;

    return false;
}

bool workerList::allHaveData() {
    int count = 0;
    int tempSize = size;
    for(node *curr = head; curr != NULL; curr = curr->next)
        if(curr->sendFD == -1)
            tempSize--;
        else if(curr->data->length() > 0)
            count++;
    
    return count == tempSize;
}

string workerList::getStats(string query) {
    if(allHaveData() == false)
        return END_READ;    // Query answer not ready yet

    string res = "";
    if(query.find("/diseaseFrequency") == string::npos) {
        for(node *curr = head; curr != NULL; curr = curr->next)
            if(curr->sendFD != -1)
                res += curr->data->dequeue();
        // Result not found
        if(res == "") res = "--Not found\n";
    }
    else {
        int freq = 0;
        for(node *curr = head; curr != NULL; curr = curr->next)
            if(curr->sendFD != -1)
                freq += stoi(curr->data->dequeue());

        res = to_string(freq) + "\n";
    }

    return res;
}

void workerList::sendMessage(string msg) {
    for(node *curr = head; curr != NULL; curr = curr->next)
        if(curr->sendFD != -1)
            ::sendMessage(curr->sendFD, msg) <= 0;
}
