#include <iostream>
#include "../HeaderFiles/PatientList.h"

using namespace std;


void PatientList::empty(node* n) {
	if (n == NULL)
	    return;

	empty(n->next);
	delete n;
}

void PatientList::deleteRecords() {
    for (node* curr = head; curr; curr = curr->next)
        delete curr->data;
}

PatientList::PatientList() {
	head = NULL;
}

PatientList::~PatientList() {
    empty(head);
}

bool PatientList::member(PatientRecord* x) {
    for (node* curr = head; curr; curr = curr->next)
        if (x->id() == curr->data->id())    // Compare id of ax and curr
            return true;

    return false;
}

bool PatientList::insert(PatientRecord* x) {
    if (member(x)) {
        cerr << "ERROR\n";
        delete x;
        return true;
    }

    node* temp = head;
    head = new node;
    head->data = x;
    head->next = temp;

    return true;
}

bool PatientList::changeExit(string id, int exit) {
    for (node* curr = head; curr; curr = curr->next)
        if (id == curr->data->id()) {
            // if(curr->data->entry() > exit)      // Exit Date must be > Entry Date
            //     cerr << "ERROR" << endl;
            // else
            //     curr->data->exitDate = exit;
                
            return true;
        }
    
    return false;
}

void PatientList::display() {
    for (node* curr = head; curr; curr = curr->next)
        cout << curr->data->id() << " " 
             << curr->data->firstName() << " " 
             << curr->data->lastName() << " " 
             << curr->data->disease() << " " 
             << curr->data->getCountry() << " "
             << curr->data->getAge() << " " 
             << curr->data->entry() << " " 
             << curr->data->exitDate << endl;
}

int PatientList::count(PatientRecord *p) {
    int counter = 0;

    for(node* curr = head; curr; curr = curr->next)
        if(curr->data->compare(p))
            counter++;
    
    return counter;
}

PatientRecord* PatientList::find(string id) {
    for(node* curr = head; curr; curr = curr->next)
        if(curr->data->id() == id)
            return curr->data;
    
    return NULL;
}

PatientRecord* PatientList::get(int index) {
    int currIndex = 0;
    for(node* curr = head; curr; curr = curr->next)
        if(currIndex++ == index)
            return curr->data;
    
    return NULL;
}

PatientRecord* PatientList::pop() {
    if(head == NULL)
        return NULL;

    PatientRecord *ret = head->data;
    node *temp = head->next;
    delete head;
    head = temp;
    return ret;
}
