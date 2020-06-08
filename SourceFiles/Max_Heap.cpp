#include <iostream>
#include "../HeaderFiles/Max_Heap.h"

using namespace std; 


Max_Heap::Max_Heap() {
    root = NULL;
}

void Max_Heap::empty(node* t) {
    if (t == NULL)
        return;

    empty(t->left);
    empty(t->right);
    delete t;
}

Max_Heap::~Max_Heap() {
    empty(root);
}

Max_Heap::node* Max_Heap::createNode(string k, int v) {  
    node* temp = new node; 
    temp->key = k;
    temp->value = v;
    temp->height = 0;
    temp->left = NULL;
    temp->right = NULL;

    return temp;  
}

int Max_Heap::height(node *t) {
    if(t == NULL)
        return -1;
    
    return t->height;
}

void Max_Heap::swap(node *t, node *u) {
    string tempStr = t->key;
    int tempValue = t->value;

    t->key = u->key;
    t->value = u->value;
    u->key = tempStr;
    u->value = tempValue;
}

Max_Heap::node* Max_Heap::recInsert(node *t, node *newNode) {
    if(t == NULL)   // Insert at the bottom
        t = newNode;
    else if(height(t->left) <= height(t->right)) {
        t->left = recInsert(t->left, newNode);
        
        if(t->value < t->left->value)   // maintain level order of max heap
            swap(t, t->left);
    } else {
        t->right = recInsert(t->right, newNode);

        if(t->value < t->right->value)  // maintain level order of max heap
            swap(t, t->right);
    }

    t->height = max(height(t->left), height(t->right)) + 1;     // Fix height of t after inserting newNode
    return t;
}

void Max_Heap::MaxHeapify(node *t) {
    node *biggest = t;
    // Swap values of nodes until max level order is achieved
    if(t->left != NULL && t->left->value > t->value)
        biggest = t->left;
    // if right > left
    if(t->right != NULL && t->right->value > biggest->value)
        biggest = t->right;
    // Repeat for swapped child
    if(biggest != t) {
        swap(biggest, t);       // Parent node > child
        MaxHeapify(biggest);    // Repeat for child
    }
}

Max_Heap::node* Max_Heap::removeBottom(node *t) {
    node *temp;

    if(height(t) < 1) {     // lowest node is root or tree is empty
        temp = root;
        root = NULL;
        return temp;
    }
    else if(height(t) == 1) // Prefer right node
        if(t->right) {
            temp = t->right;
            t->right = NULL;
        }
        else {
            temp = t->left;
            t->left = NULL;
        }
    // Go to node with greatest height to maintain balance
    else if(height(t->left) > height(t->right)) 
        temp = removeBottom(t->left);
    else
        temp = removeBottom(t->right);
    // Fix height of t
    t->height = max(height(t->left), height(t->right)) + 1;

    return temp;
}

string Max_Heap::extractMax() {
    string ret = "";

    if(height(root) < 0)    // if root == NULL
        return ret;
    // Print root data
    ret = root->key + ": " + to_string(root->value) + "%\n";
    // Delete root
    if(height(root) == 0) {
        delete root;
        root = NULL;
    } else {    // Replace root with bottom node and fix heap
        node *temp = removeBottom(root);
        swap(root, temp);
        MaxHeapify(root);
        delete temp;
    }

    return ret;
}

void Max_Heap::insert(string k, int v) {
    root = recInsert(root, createNode(k, v));
}

bool Max_Heap::isEmpty() {
    return root == NULL;
};