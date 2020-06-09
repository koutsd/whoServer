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

void workerList::insert(int fd) {
    size++;

    node* temp = head;
    head = new node;
    head->fd = fd;
    head->data = new strList();
    head->next = temp;
}

void workerList::remove(int fd) {
    if(size == 0)
        return;
        
    node **curr = &head;
    for(curr; (*curr)->next != NULL; curr = &(*curr)->next);
        if(fd == (*curr)->fd) {
            node **temp = curr;
            *curr = (*curr)->next;
            // delete (*temp)->data;
            delete *temp;
            size--;
            return;
        }
}

int workerList::length() { 
    return size;
}

strList* workerList::getData(int fd) { 
    for(node *curr = head; curr != NULL; curr = curr->next)
        if(fd == curr->fd)
            return curr->data;

    return NULL;
}

void workerList::addData(int fd, string data) { 
    for(node *curr = head; curr != NULL; curr = curr->next)
        if(fd == curr->fd) {
            curr->data->add(data);
            return;
        }
}

bool workerList::member(int fd) {
    for(node *curr = head; curr != NULL; curr = curr->next)
        if(fd == curr->fd)
            return true;
    
    return false;
}

bool workerList::allHaveData() {
    int count = 0;
    for(node *curr = head; curr != NULL; curr = curr->next)
        if(curr->data->length() > 0)
            count++;
    
    return count == size;
}

string workerList::removeDatafromIndex(int target) {
    int index = 0;
    for(node *curr = head; curr != NULL; curr = curr->next)
        if(index++ == target)
            return curr->data->dequeue();
    
    return "";
}

bool workerList::isLast(int fd) {
    if(size) {
        node *curr = head;
        for(curr; curr->next != NULL; curr = curr->next);
        return fd == curr->fd;
    }

    return false;
}