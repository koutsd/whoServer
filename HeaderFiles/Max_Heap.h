#pragma once
#include <string>

using namespace std;


class Max_Heap {
private:
    struct node {
        string key;
        int value, height;
        node *left, *right;
    };

    node *root;

    void empty(node *t);            // Delete all nodes
    node* createNode(string k, int v);      // Create new node with key = k and value = v
    node* recInsert(node *root, node *newNode);     // Insert newNode respecting Level order of max heap
    int height(node *t);            // return height of node in tree
    void swap(node *t, node *u);    // Swap values between 2 nodes
    void MaxHeapify(node *t);       // Fix level order of max heap
    node* removeBottom(node *t);    // Remove and return a node from the bottom of the tree
    
public: 
    Max_Heap(); 
    ~Max_Heap();
    bool isEmpty();                 // Check if tree has 0 nodes
    void insert(string k, int v);
    string extractMax();             // Remove and print root
}; 