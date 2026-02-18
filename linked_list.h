// Group 4 Project 2

// Linked list definitions for managing customers @Qingzheng

#ifndef PROJECT_2_LINKED_LIST_H
#define PROJECT_2_LINKED_LIST_H

#include "customer.h"
#include <stdbool.h>

typedef struct Node {
    Customer* customer;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    int size;
} LinkedList, Queue;

LinkedList* linked_list_create();
void linked_list_destroy(LinkedList* list);
void linked_list_push_front(LinkedList* list, Customer* customer);
void linked_list_push_back(LinkedList* list, Customer* customer);
Customer* linked_list_pop_front(LinkedList* list);
Customer* linked_list_pop_back(LinkedList* list);
Customer* linked_list_peek_front(LinkedList* list);
Customer* linked_list_peek_back(LinkedList* list);
int linked_list_size(LinkedList* list);
bool linked_list_is_empty(LinkedList* list);

Queue* queue_create();
void queue_destroy(Queue* queue);
void queue_enqueue(Queue* queue, Customer* customer);
Customer* queue_dequeue(Queue* queue);
Customer* queue_front(Queue* queue);
int queue_size(Queue* queue);
bool queue_is_empty(Queue* queue);

#endif //PROJECT_2_LINKED_LIST_H