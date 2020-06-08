#include <iostream>
#include "../HeaderFiles/Hash_Table.h"
#include "../HeaderFiles/Max_Heap.h"

using namespace std;


void Hash_Table::Bucket::empty(node* n) {
    if(n == NULL)
        return;

    empty(n->next);     // Empty next node
    delete n->tree;     // Delete AVL Tree
    delete n;           // Delete node
}

Hash_Table::Bucket::Bucket() {
    head = NULL;
}

Hash_Table::Bucket::~Bucket() {
    empty(head);
}

void Hash_Table::Bucket::insert(PatientRecord* x, string key) {
    for (node* curr = head; curr; curr = curr->next)        // find node with key
        if (curr->key == key) {
            curr->tree->insert(x);      // Insert x in AVL Tree
            return;
        }

    node* temp = head;
    head = new node;
    head->tree = new AVL_Tree();
    head->key = key;
    head->tree->insert(x);
    head->next = temp;
}


int Hash_Table::hash(string key) {      // Hash Func
    int sum = 0;

    for(int c = 0; c < key.length(); c++)
        sum += (c + 1) * key[c];

    return sum % size;
}

Hash_Table::Hash_Table(int tableSize, string key) {
    if(tableSize == 0) {
        cout << "- Hash Table Size must be greater than " << 0 << endl;
        exit(1);
    } 

    // Create Hash Table of tableSize size
    size = tableSize;
    keyType = key;      // keyType = Country/Disease
    table = new Bucket * [size];
    for (int b = 0; b < size; b++)
        table[b] = new Bucket();
}

Hash_Table::~Hash_Table() {
    for (int b = 0; b < size; b++)
        delete table[b];
    
    delete[] table;
}

void Hash_Table::insert(PatientRecord* x) {
    string key;

    if (keyType == "Disease")       // Disease HT
        key = x->disease();
    else if (keyType == "Country")  // Country HT
        key = x->getCountry();

    table[hash(key)]->insert(x, key);
}

// Query Funcions
void Hash_Table::numCurrentPatients(string key) {
    PatientRecord *temp = new PatientRecord("", "", "", "", "", -1, -1, 0);     // Dummy PatientRecord with exit date = 0 (no exit date)

    if(key == "")   // Print numCurrentPatients for all keys
        for(int b = 0; b < size; b++)
            for (Bucket::node* curr = table[b]->head; curr; curr = curr->next)
                cout << curr->key << " " << curr->tree->count(0, 0, temp) << endl;
    else {           // Print only for assigned key
        int count = 0;
        
        for (Bucket::node* curr = table[hash(key)]->head; curr; curr = curr->next)
            if(key == curr->key) {
                count = curr->tree->count(0, 0, temp);
                break;
            }
        
        cout << key << " " << count << endl;
    }
            
    cout << endl;
    delete temp;
}

void Hash_Table::globalStats(int date1, int date2) {
    for(int b = 0; b < size; b++)
        for (Bucket::node* curr = table[b]->head; curr; curr = curr->next)
            cout << curr->key << " " << curr->tree->count(date1, date2) << endl;
    
    cout << endl;
}

int Hash_Table::frequency(string key, int date1, int date2, PatientRecord *p) {
    int count = 0;

    for (Bucket::node* curr = table[hash(key)]->head; curr; curr = curr->next)
        if(key == curr->key) {  // Find node in Bucket with assigned key
            count = curr->tree->count(date1, date2, p);
            break;
        }
    
    return count;
}

void Hash_Table::topK(int k, int date1, int date2, PatientRecord *p) {
    Max_Heap *h = new Max_Heap();   // Create empty Binary heap

    for(int i = 0; i < size; i++)
        for (Bucket::node* curr = table[i]->head; curr; curr = curr->next)
            h->insert(curr->key, curr->tree->count(date1, date2, p));

    for(int i = 0; i < k && !h->isEmpty(); i++)
        h->extractMax();
    
    cout << endl;
    delete h;
}

void Hash_Table::changeExitDate(string id, int date) {
    for(int i = 0; i < size; i++)   // Search HT for Patient record with id
        for (Bucket::node* curr = table[i]->head; curr; curr = curr->next)
            if(curr->tree->changeExitDate(id, date))
                return;
}

void Hash_Table::deleteRecords() {
    for(int i = 0; i < size; i++)   // Delete all PatientRecords
        for (Bucket::node* curr = table[i]->head; curr; curr = curr->next)
            curr->tree->deleteRecords();
}

string Hash_Table::summary(int date) {
    string str = "";
    int count;

    for(int b = 0; b < size; b++)
        for (Bucket::node* curr = table[b]->head; curr; curr = curr->next) {
            PatientRecord *p1, *p2;
            str += curr->key;

            p1 = new PatientRecord("", "", "", "", "", 0, -1, -1);
            p2 = new PatientRecord("", "", "", "", "", 10, -1, -1);
            count = curr->tree->count(date, date, p1) + curr->tree->count(date, date, p2);
            delete p1;
            delete p2;
            str += "\nAge range 0-20 years: " + to_string(count);

            p1 = new PatientRecord("", "", "", "", "", 20, -1, -1);
            p2 = new PatientRecord("", "", "", "", "", 30, -1, -1);
            count = curr->tree->count(date, date, p1) + curr->tree->count(date, date, p2);
            delete p1;
            delete p2;
            str += "\nAge range 20-40 years: " + to_string(count);

            p1 = new PatientRecord("", "", "", "", "", 40, -1, -1);
            p2 = new PatientRecord("", "", "", "", "", 50, -1, -1);
            count = curr->tree->count(date, date, p1) + curr->tree->count(date, date, p2);
            delete p1;
            delete p2;
            str += "\nAge range 40-60 years: " + to_string(count);

            p1 = new PatientRecord("", "", "", "", "", 60, -1, -1);
            count = curr->tree->count(date, date, p1);
            delete p1;
            str += "\nAge range 60+ years: " + to_string(count)
                + "\n\n";
        }

    return str;
}

PatientRecord* Hash_Table::find(string id) {
    PatientRecord *res; 

    for(int b = 0; b < size; b++)
        for (Bucket::node* curr = table[b]->head; curr; curr = curr->next)
            if((res = curr->tree->find(id)) != NULL)
                return res;

    return NULL;
}

string Hash_Table::topK_AgeRanges(int k, string disease, int date1, int date2) {
    Max_Heap *h = new Max_Heap();   // Create empty Binary heap
    
    int count0_20 = 0, count20_40 = 0, count40_60 = 0, count60 = 0;

    for(int i = 0; i < size; i++)
        for (Bucket::node* curr = table[i]->head; curr; curr = curr->next)
            if(curr->key == disease) {
                PatientRecord *p = new PatientRecord("", "", "", "", "", -1, -1, -1);
                int count, total = curr->tree->count(date1, date2, p);
                delete p;

                if(!total) return "";

                p = new PatientRecord("", "", "", "", "", 0, -1, -1);
                h->insert("0-10", (curr->tree->count(date1, date2, p) * 100) / total);
                delete p;

                p = new PatientRecord("", "", "", "", "", 10, -1, -1);
                h->insert("10-20", (curr->tree->count(date1, date2, p) * 100) / total);
                delete p;

                p = new PatientRecord("", "", "", "", "", 20, -1, -1);
                h->insert("20-30", (curr->tree->count(date1, date2, p) * 100) / total);
                delete p;

                p = new PatientRecord("", "", "", "", "", 30, -1, -1);
                h->insert("30-40", (curr->tree->count(date1, date2, p) * 100) / total);
                delete p;

                p = new PatientRecord("", "", "", "", "", 40, -1, -1);
                h->insert("40-50", (curr->tree->count(date1, date2, p) * 100) / total);
                delete p;

                p = new PatientRecord("", "", "", "", "", 50, -1, -1);
                h->insert("50-60", (curr->tree->count(date1, date2, p) * 100) / total);
                delete p;

                p = new PatientRecord("", "", "", "", "", 60, -1, -1);
                h->insert("60+", (curr->tree->count(date1, date2, p) * 100) / total);
                delete p;

                break;
            }

    string ret = "";
    for(int i = 0; i < k && !h->isEmpty(); i++)
        ret += h->extractMax();
    
    delete h;
    return ret;
}
