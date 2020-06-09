#include <iostream>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include "../HeaderFiles/Util.h"

#define BUFFER_SIZE 256

using namespace std;


void sendMessage(int pipe, string str, int buffSize) {
    int bufferSize = buffSize ? buffSize : BUFFER_SIZE;

    char msg[bufferSize] = {0};
    int splitLength = (bufferSize - 1);
    int NumSubstrings = str.length() / splitLength;

    for (int i = 0; i < NumSubstrings; i++) {
        strcpy(msg, str.substr(i * splitLength, splitLength).c_str());
        write(pipe, msg, bufferSize);
    }

    if (str.length() % splitLength != 0) {
        strcpy(msg, str.substr(splitLength * NumSubstrings).c_str());
        write(pipe, msg, bufferSize);
    }

    strcpy(msg, "^");
    write(pipe, msg, bufferSize);
}


string receiveMessage(int pipe, int buffSize) {
    int bufferSize = buffSize ? buffSize : BUFFER_SIZE;

    char msg[bufferSize] = {0};
    read(pipe, msg, bufferSize);

    string data = "";
    while(strcmp(msg, "^")) {
        data += msg;
        read(pipe, msg, bufferSize);
    }

    return data;
}


int dateToInt(string date) {
    if(date == "-" || date == "")   // empty date = 0
        return 0;

    string temp;
    stringstream dateStream(date);

    getline(dateStream, temp, '-');
    int d = stoi(temp);
    getline(dateStream, temp, '-');
    int m = stoi(temp);
    getline(dateStream, temp, '-');
    int y = stoi(temp);

    return y*10000 + m*100 + d;     // date = YYYYMMDD
}

string intToDate(int date) {
    if(date == -1 || date == 0)
        return "--";

    string dateStr = to_string(date);
    return dateStr.substr(6, 2) + "-" + dateStr.substr(4, 2) + "-" + dateStr.substr(0, 4);
}


bool isDate(string str) {
    if(str.length() != 10)
        return false;
    
    string temp = str.substr(0,2);
    string::const_iterator it = temp.begin();
    while (it != temp.end() && isdigit(*it)) ++it;
    if(temp.empty() || it != temp.end())
        return false;
    
    if(str.substr(2,1) != "-")
        return false;

    temp = str.substr(3,2);
    it = temp.begin();
    while (it != temp.end() && isdigit(*it)) ++it;
    if(temp.empty() || it != temp.end())
        return false;

    if(str.substr(5,1) != "-")
        return false;
    
    temp = str.substr(6,4);
    it = temp.begin();
    while (it != temp.end() && isdigit(*it)) ++it;
    if(temp.empty() || it != temp.end())
        return false;

    return true;
}


strList::strList() {
    head = NULL;
    size = 0;
}

void strList::empty(node* n) {
	if (n == NULL)
	    return;

	empty(n->next);
	delete n;
}

strList::~strList() {
    empty(head);
}

int strList::length() {
    return size;
}

void strList::add(string v) {
    size++;

    node* temp = head;
    head = new node;
    head->value = v;
    head->next = temp;
}

string strList::get(int index) {
    if(index >= size || size == 0)
        return "";

    node *curr = head;
    for(int i = 0; i < index; i++)
        curr = curr->next;

    return curr->value;
}

bool strList::member(string v) {
    node *curr = head;
    for(int i = 0; i < size; i++) {
        if(curr->value == v)
            return true;

        curr = curr->next;
    }
        
    return false;
}

string strList::dequeue() {
    if(size == 0)
        return "";

    node **curr = &head;
    for(curr; (*curr)->next != NULL; curr = &(*curr)->next);

    string value = (*curr)->value;

    delete *curr;
    *curr = NULL;
    size--;

    return value;
}


intList::intList() {
    head = NULL;
    size = 0;
}

void intList::empty(node* n) {
	if (n == NULL)
	    return;

	empty(n->next);
	delete n;
}

intList::~intList() {
    empty(head);
}

int intList::length() {
    return size;
}

void intList::add(int v) {
    size++;

    node* temp = head;
    head = new node;
    head->value = v;
    head->next = temp;
}

int intList::get(int index) {
    if(index >= size || size == 0)
        return -1;

    node *curr = head;
    for(int i = 0; i < index; i++)
        curr = curr->next;

    return curr->value;
}

bool intList::member(int v) {
    node *curr = head;
    for(int i = 0; i < size; i++) {
        if(curr->value == v)
            return true;

        curr = curr->next;
    }
        
    return false;
}

int intList::dequeue() {
    if(size == 0)
        return -1;

    node **curr = &head;
    for(curr; (*curr)->next != NULL; curr = &(*curr)->next);

    int value = (*curr)->value;

    delete *curr;
    *curr = NULL;
    size--;
    
    return value;
}
