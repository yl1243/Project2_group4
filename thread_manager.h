// Group 4 Project 2

// Exposes the thread manager interface and test-facing data structures @Qingzheng

#ifndef PROJECT_2_THREAD_MANAGER_H
#define PROJECT_2_THREAD_MANAGER_H

#include <pthread.h>
#include "customer.h"

typedef struct
{
    int total_customers;
    int total_stair_steps;

    unsigned int seed;           // Used in lottery scheduling
    int max_batch_size;          // Max number of customers allowed in one batch
    int max_consecutive_batches; // Max number of consecutive batches in the same direction to prevent starvation

    // =======================@Yujie
    int unit_usec; // time per step in microseconds， total time appx = S * unit_sec
    // ==========================

} StairConfig;

typedef struct ThreadManager ThreadManager;

ThreadManager *thread_manager_create(const StairConfig *config);
void thread_manager_destroy(ThreadManager *manager);

// Execute: create every thread for each customer and join
void thread_manager_run(ThreadManager *manager, Customer *customers);

#endif // PROJECT_2_THREAD_MANAGER_H
