#include "thread_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// testcases @Yujie
static Direction rand_dir()
{
    return (rand() % 2 == 0) ? DIRECTION_UP : DIRECTION_DOWN;
}

int main(int argc, char **argv)
{
    if (argc < 7)
    {
        printf("Usage: %s N_customers S_steps seed max_batch max_consec unit_usec\n", argv[0]);
        printf("Example: %s 10 13 42 4 2 10000\n", argv[0]);
        return 1;
    }

    StairConfig cfg;
    cfg.total_customers = atoi(argv[1]);
    cfg.total_stair_steps = atoi(argv[2]);
    cfg.seed = (unsigned int)atoi(argv[3]);
    cfg.max_batch_size = atoi(argv[4]);
    cfg.max_consecutive_batches = atoi(argv[5]);
    cfg.unit_usec = atoi(argv[6]);

    if (cfg.total_customers <= 0 || cfg.total_stair_steps < 0)
    {
        printf("Invalid N or S.\n");
        return 1;
    }

    srand(cfg.seed);

    Customer *customers = (Customer *)calloc((size_t)cfg.total_customers, sizeof(Customer));
    if (!customers)
        return 1;

    // Generate a simple workload
    // arrival_time: random 0..(N/2)，direction random
    for (int i = 0; i < cfg.total_customers; i++)
    {
        char *name = (char *)malloc(16);
        snprintf(name, 16, "C%d", i);

        customers[i].name = name;
        customers[i].direction = rand_dir();
        customers[i].arrival_time = rand() % (cfg.total_customers / 2 + 1);

        // stats init will be done in thread_manager_run
    }

    ThreadManager *m = thread_manager_create(&cfg);
    thread_manager_run(m, customers);
    thread_manager_destroy(m);

    // cleanup names
    for (int i = 0; i < cfg.total_customers; i++)
    {
        free(customers[i].name);
    }
    free(customers);

    return 0;
}
