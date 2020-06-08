#pragma once
#include "PatientRecord.h"


class PatientList {
private:
    struct node {
        PatientRecord* data;
        node* next;
    };

    node* head;

    void empty(node* n);    // Delete all nodes of list

public:
    PatientList();
    ~PatientList();
    bool member(PatientRecord* x);      // Check if x is in list
    bool insert(PatientRecord* x);
    bool changeExit(string id, int exit);  // Change exitDate of record with recordID = id
    void display();                     // Display all records in list
    void deleteRecords();               // delete all records
    int count(PatientRecord *p=NULL);   // Count all records similar to p
    PatientRecord* find(string id);
    PatientRecord* get(int index);
    PatientRecord* pop();               // Remove first node and return data
};

