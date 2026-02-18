// Group 4 Project 2

#include "linked_list.h"
#include <stdlib.h>


// Create a new node @Qingzheng
static Node* node_create(Customer* customer) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (!node) {
        return NULL;
    }
    node->customer = customer;
    node->next = NULL;
    return node;
}

// [Exposed]
// Create a linked list @Qingzheng
LinkedList* linked_list_create() {
    LinkedList* list = (LinkedList*)malloc(sizeof(LinkedList));
    if (!list) {
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

// [Exposed]
// Destroy a linked list and free all associated memory @Qingzheng
void linked_list_destroy(LinkedList* list) {
    if (!list) {
        return;
    }
    Node* current = list->head;
    while (current) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

// [Exposed]
// Push a customer to the front of the linked list @Qingzheng
void linked_list_push_front(LinkedList* list, Customer* customer) {
    if (!list) {
        return;
    }
    Node* node = node_create(customer);
    if (!node) {
        return;
    }
    node->next = list->head;
    list->head = node;
    if (!list->tail) {
        list->tail = node;
    }
    list->size += 1;
}

// [Exposed]
// Push a customer to the back of the linked list @Qingzheng
void linked_list_push_back(LinkedList* list, Customer* customer) {
    if (!list) {
        return;
    }
    Node* node = node_create(customer);
    if (!node) {
        return;
    }
    if (!list->tail) {
        list->head = node;
        list->tail = node;
        list->size = 1;
        return;
    }
    list->tail->next = node;
    list->tail = node;
    list->size += 1;
}

// [Exposed]
// Pop a customer from the front of the linked list and return it @Qingzheng
Customer* linked_list_pop_front(LinkedList* list) {
    if (!list || !list->head) {
        return NULL;
    }
    Node* node = list->head;
    Customer* customer = node->customer;
    list->head = node->next;
    if (!list->head) {
        list->tail = NULL;
    }
    free(node);
    if (list->size > 0) {
        list->size -= 1;
    }
    return customer;
}

// [Exposed]
// Pop a customer from the back of the linked list and return it @Qingzheng
Customer* linked_list_pop_back(LinkedList* list) {
    if (!list || !list->head) {
        return NULL;
    }
    if (list->head == list->tail) {
        Customer* customer = list->head->customer;
        free(list->head);
        list->head = NULL;
        list->tail = NULL;
        if (list->size > 0) {
            list->size -= 1;
        }
        return customer;
    }

    Node* current = list->head;
    while (current->next != list->tail) {
        current = current->next;
    }
    Customer* customer = list->tail->customer;
    free(list->tail);
    list->tail = current;
    list->tail->next = NULL;
    if (list->size > 0) {
        list->size -= 1;
    }
    return customer;
}

// [Exposed]
// Peek at the front customer of the linked list without removing it @Qingzheng
Customer* linked_list_peek_front(LinkedList* list) {
    if (!list || !list->head) {
        return NULL;
    }
    return list->head->customer;
}

// [Exposed]
// Peek at the back customer of the linked list without removing it @Qingzheng
Customer* linked_list_peek_back(LinkedList* list) {
    if (!list || !list->tail) {
        return NULL;
    }
    return list->tail->customer;
}

// [Exposed]
// Get the size of the linked list @Qingzheng
int linked_list_size(LinkedList* list) {
    if (!list) {
        return 0;
    }
    return list->size;
}

// [Exposed]
// Check if the linked list is empty @Qingzheng
bool linked_list_is_empty(LinkedList* list) {
    return !list || list->head == NULL;
}

// [Exposed]
// Alias for linked_list_create
Queue* queue_create() {
    return linked_list_create();
}

// [Exposed]
// Alias for linked_list_destroy
void queue_destroy(Queue* queue) {
    linked_list_destroy(queue);
}

// [Exposed]
// Alias for linked_list_push_back
void queue_enqueue(Queue* queue, Customer* customer) {
    linked_list_push_back(queue, customer);
}

// [Exposed]
// Alias for linked_list_pop_front
Customer* queue_dequeue(Queue* queue) {
    return linked_list_pop_front(queue);
}

// [Exposed]
// Alias for linked_list_peek_front
Customer* queue_front(Queue* queue) {
    return linked_list_peek_front(queue);
}

// [Exposed]
// Alias for linked_list_size
int queue_size(Queue* queue) {
    return linked_list_size(queue);
}

// [Exposed]
// Alias for linked_list_is_empty
bool queue_is_empty(Queue* queue) {
    return linked_list_is_empty(queue);
}
