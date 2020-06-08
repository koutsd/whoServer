#pragma once
#include "AVL_Tree.h"

class Hash_Table {
private:
    class Bucket {      // Store all data with the same hash value
    public:
        struct node {
            string key;
            AVL_Tree* tree;
            node* next;
        };

        node* head;

        void empty(node* n);    // Delete all Data Structures in Bucket
        Bucket();
        ~Bucket();
        void insert(PatientRecord* x, string key);
    };

    Bucket** table;
    string keyType;
    int size;

    int hash(string key);

public:
    Hash_Table(int tableSize, string key);
    ~Hash_Table();
    void deleteRecords();       // Delete all PatientRecords
    void insert(PatientRecord* x);
    // Query Funcions
    void numCurrentPatients(string key="");     // Count patienRecords with no exit date
    void globalStats(int date1=0, int date2=0);     // Count PatientRecords of all keys (date1 < entryDate < date2)
    int frequency(string key, int date1, int date2, PatientRecord *p=NULL);    // Count PatientRecords with key that have attributes of p
    void topK(int k, int date1, int date2, PatientRecord *p);      // Print k keys with the most PatientRecrds (date1 < entryDate < date2)
    void changeExitDate(string id, int date);      // Change ExitDate of PatientRecord with id
    PatientRecord* find(string id);
    string topK_AgeRanges(int k, string disease, int date1, int date2);
    string summary(int date);

};