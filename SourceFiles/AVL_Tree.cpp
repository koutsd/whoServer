#include <iostream>
#include "../HeaderFiles/AVL_Tree.h"

using namespace std;

void AVL_Tree::empty(node* t) {
    if (t == NULL)
        return;

    empty(t->left);     // empty left node
    empty(t->right);    // empty right node
    delete t->list;     // delete list data structure
    delete t;           // delete node
}

void AVL_Tree::deleteRecordsRec(node* t) {
    if (t == NULL)
        return;

    deleteRecordsRec(t->left);  // delete records of left subtree
    deleteRecordsRec(t->right); // delete records of right subtree
    t->list->deleteRecords();   // delete records in list
}

void AVL_Tree::deleteRecords() {
    deleteRecordsRec(root);
}

AVL_Tree::node* AVL_Tree::recInsert(PatientRecord* x, node* t) {
    if (t == NULL) {    // Insert at the bottom
        t = new node;
        t->list = new PatientList();
        t->list->insert(x);
        t->height = 0;
        t->date = x->entry();
        t->left = t->right = NULL;
    }
    else if (x->entry() < t->date) {    // go left
        t->left = recInsert(x, t->left);
        if (height(t->left) - height(t->right) == 2)    // fix balance
            if (x->entry() < t->left->date)
                t = RightRotate(t);
            else {  // Double rotate
                t->left = LeftRotate(t->left);
                t = RightRotate(t);
            }
    }
    else if (x->entry() > t->date) {    // go right
        t->right = recInsert(x, t->right);
        if (height(t->right) - height(t->left) == 2)    // fix balance
            if (x->entry() > t->right->date)
                t = LeftRotate(t);
            else {  // Double rotate
                t->right = RightRotate(t->right);
                t = LeftRotate(t);
            }
    }
    else    // if node with x->entryDate already exist
        t->list->insert(x);     // insert to list
    // Fix height of nodes if necessary
    t->height = max(height(t->left), height(t->right)) + 1;
    return t;
}

AVL_Tree::node* AVL_Tree::RightRotate(node*& t) {
    node* u = t->left;
    t->left = u->right;
    u->right = t;
    t->height = max(height(t->left), height(t->right)) + 1;
    u->height = max(height(u->left), t->height) + 1;

    return u;
}

AVL_Tree::node* AVL_Tree::LeftRotate(node*& t) {
    node* u = t->right;
    t->right = u->left;
    u->left = t;
    t->height = max(height(t->left), height(t->right)) + 1;
    u->height = max(height(t->right), t->height) + 1;

    return u;
}

int AVL_Tree::AVL_Tree::height(node* t) {
    if(t == NULL)
        return -1;

    return t->height;
}

int AVL_Tree::AVL_Tree::recCount(node* t, int date1, int date2, PatientRecord *p) {
    if(t == NULL)   // If no node found with date1 < date < date2  count = 0
        return 0;
    // if node matches requested dates
    if((date1 == 0 && date2 == 0) || (t->date >= date1 && t->date <= date2))
        return t->list->count(p) + recCount(t->left, date1, date2, p) + recCount(t->right, date1, date2, p);
    if(t->date < date1)     // Search for later dates
        return recCount(t->right, date1, date2, p);
    if(t->date > date2)     // Search for earlier dates
        return recCount(t->left, date1, date2, p);

    return -1;
}

PatientRecord* AVL_Tree::AVL_Tree::recFind(node* t, string id) {
    if(t == NULL)   // If no node found with date1 < date < date2  count = 0
        return NULL;
    
    PatientRecord *res = t->list->find(id);
    res = res == NULL ? recFind(t->left, id) : res;
    return res == NULL ? recFind(t->right, id) : res;
}

AVL_Tree::node* AVL_Tree::recInsert_2(PatientRecord* x, node* t) {
    if (t == NULL) {    // Insert at the bottom
        t = new node;
        t->list = new PatientList();
        t->list->insert(x);
        t->height = 0;
        t->date = x->exitDate;
        t->left = t->right = NULL;
    }
    else if (x->exitDate < t->date) {    // go left
        t->left = recInsert_2(x, t->left);
        if (height(t->left) - height(t->right) == 2)    // fix balance
            if (x->exitDate < t->left->date)
                t = RightRotate(t);
            else {  // Double rotate
                t->left = LeftRotate(t->left);
                t = RightRotate(t);
            }
    }
    else if (x->exitDate > t->date) {    // go right
        t->right = recInsert_2(x, t->right);
        if (height(t->right) - height(t->left) == 2)    // fix balance
            if (x->exitDate > t->right->date)
                t = LeftRotate(t);
            else {  // Double rotate
                t->right = RightRotate(t->right);
                t = LeftRotate(t);
            }
    }
    else    // if node with x->entryDate already exist
        t->list->insert(x);     // insert to list
    // Fix height of nodes if necessary
    t->height = max(height(t->left), height(t->right)) + 1;
    return t;
}


AVL_Tree::AVL_Tree() {
    root = NULL;
}

AVL_Tree::~AVL_Tree() {
    empty(root);
}

void AVL_Tree::insert(PatientRecord* x) {
    root = recInsert(x, root);
}

void AVL_Tree::insert_2(PatientRecord* x) {
    root = recInsert_2(x, root);
}

int AVL_Tree::count(int date1, int date2, PatientRecord *p) {
    return recCount(root, date1, date2, p);
}

bool AVL_Tree::changeExitRec(node *t, string id, int date) {
    if(t == NULL)
        return false;
    
    return t->list->changeExit(id, date) || changeExitRec(t->left, id, date) || changeExitRec(t->right, id, date);
}

bool AVL_Tree::changeExitDate(string id, int date) {
    return changeExitRec(root, id, date);
}

PatientRecord* AVL_Tree::find(string id) {
    return recFind(root, id);
}