#pragma once
#include <string>
#include "../HeaderFiles/Util.h"

using namespace std;


extern const string END_READ;

int sendMessage(int pipe, string str, int bufferSize=0);

string receiveMessage(int pipe, int bufferSize=0);

int dateToInt(string date);

string intToDate(int date);

bool isDate(string str);


class strList {
private:
    struct node {
        string value;
        node *next;
    };

    node *head;
    int size;

    void empty(node* n);

public:
    strList();
    ~strList();
    int length();
    void add(string v);
    bool member(string v);
    string get(int index);
    string dequeue();
};


class intList {
private:
    struct node {
        int value;
        node *next;
    };

    node *head;
    int size;

    void empty(node* n);

public:
    intList();
    ~intList();
    int length();
    void add(int v);
    bool member(int v);
    int get(int index);
    int dequeue();
    void remove(int v);
};
