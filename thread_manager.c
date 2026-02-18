// Group 4 Project 2

/* Ideas:
Prevents deadlock: one direction at a time.
Prevents starvation: use bounded lottery scheduling
Efficiency: batch size proportional to demand
 */

#include "thread_manager.h"
#include "linked_list.h"
#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <semaphore.h>
#include <unistd.h> // usleep
#include <time.h>

// ADD THESE PROTOTYPES HERE
static long long now_usec(void);
static int now_tick(ThreadManager *manager);

// The manager shared by all threads @Qingzheng
struct ThreadManager

{
    const StairConfig *config;

    // @Yujie =====================
    // step semaphores — every step has one
    sem_t *step_semaphores;

    // current number of people on stairs
    int on_stairs_count;

    // waiting counters
    int waiting_up_count;
    int waiting_down_count;

    // simulation start time
    long long start_time;
    // ============================

    // pthread variables:
    pthread_mutex_t mutex;
    pthread_cond_t stairs_state_changed; // condition var to sleep/wake waiting threads
    pthread_mutex_t print_mutex;         // prevent interleaved printing

    // Empty-initialized variables:
    Direction current_direction;

    // Queues
    Queue *on_stairs_customers;
    Queue *waiting_to_go_up_customers;
    Queue *waiting_to_go_down_customers;

    // Track: each batch can only allow a certain number of customers to pass.
    // This variable tracks how many customers that are in the current batch are not yet on the stairs.
    int current_patch_not_on_stairs_yet_count;

    // Track: how many consecutive same direction batches (to prevent starvation)
    int current_direction_batch_count;
};

// [Exposed]
// Create a thread manager @Qingzheng
ThreadManager *thread_manager_create(const StairConfig *config)
{
    ThreadManager *manager = (ThreadManager *)malloc(sizeof(ThreadManager));
    manager->config = config;

    // Initialize pthread variables:
    pthread_mutex_init(&manager->mutex, NULL);
    pthread_cond_init(&manager->stairs_state_changed, NULL);
    pthread_mutex_init(&manager->print_mutex, NULL);

    // Initialize Empty-initialized variables:
    manager->current_direction = DIRECTION_NONE;

    // Initialize queues:
    manager->on_stairs_customers = queue_create();
    manager->waiting_to_go_up_customers = queue_create();
    manager->waiting_to_go_down_customers = queue_create();

    // @Yujie ============================
    // Initialize step semaphores (each stair holds only 1 customer)
    int S = config->total_stair_steps;
    manager->step_semaphores = malloc(sizeof(sem_t) * S);

    for (int i = 0; i < S; i++)
        sem_init(&manager->step_semaphores[i], 0, 1);

    // counters
    manager->on_stairs_count = 0;
    manager->waiting_up_count = 0;
    manager->waiting_down_count = 0;
    manager->current_patch_not_on_stairs_yet_count = 0; // batch remaining
    manager->current_direction_batch_count = 0;         // consecutive batches
    // 否则默认是随机垃圾值，后面逻辑会乱

    // start clock
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    manager->start_time = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
    // ====================================

    // Set random seed for lottery scheduling
    srand(config->seed);

    return manager;
}

// [Exposed]
// Destroy a thread manager @Qingzheng
void thread_manager_destroy(ThreadManager *manager)
{
    // Destroy pthread variables:
    pthread_mutex_destroy(&manager->mutex);
    pthread_cond_destroy(&manager->stairs_state_changed);
    pthread_mutex_destroy(&manager->print_mutex);

    // Destroy queues:
    queue_destroy(manager->on_stairs_customers);
    queue_destroy(manager->waiting_to_go_up_customers);
    queue_destroy(manager->waiting_to_go_down_customers);

    // @Yujie ===================================
    // Destroy and release semaphore
    int S = manager->config->total_stair_steps;
    for (int i = 0; i < S; i++)
        sem_destroy(&manager->step_semaphores[i]);

    free(manager->step_semaphores);
    // =============================================

    free(manager);
}

// Logging @Qingzheng
void print(ThreadManager *manager, const char *format, ...)
{
    pthread_mutex_lock(&manager->print_mutex);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    pthread_mutex_unlock(&manager->print_mutex);
}

// Main Simulation Logic @Qingzheng
void simulate(ThreadManager *manager, Queue *incoming_customers, int tick)
{
    // If there are customers arriving at this tick, add them to the waiting counts and print their arrival
    while (!queue_is_empty(incoming_customers) && queue_front(incoming_customers)->arrival_time <= tick)
    {
        Customer *customer = queue_dequeue(incoming_customers);
        print(manager, "Tick %d: Customer %s arrives waiting to go %s\n", now_tick(manager), customer->name,
              customer->direction == DIRECTION_UP ? "UP" : "DOWN");

        if (customer->direction == DIRECTION_UP)
        {
            queue_enqueue(manager->waiting_to_go_up_customers, customer);
        }
        else if (customer->direction == DIRECTION_DOWN)
        {
            queue_enqueue(manager->waiting_to_go_down_customers, customer);
        }
    }

    // Is the last batch finished?
    if (queue_is_empty(manager->on_stairs_customers))
    {
        // Decide the next direction based on waiting customers and batch count
        Direction next_direction = (rand() % 2 == 0) ? DIRECTION_UP : DIRECTION_DOWN;
        if (queue_is_empty(manager->waiting_to_go_up_customers))
        {
            next_direction = DIRECTION_DOWN;
        }
        else if (queue_is_empty(manager->waiting_to_go_down_customers))
        {
            next_direction = DIRECTION_UP;
        }
        else if (manager->current_direction == next_direction && manager->current_direction_batch_count >= manager->config->max_consecutive_batches)
        {
            next_direction = -next_direction; // Force switch direction to prevent starvation
        }
        if (next_direction != manager->current_direction)
        {
            manager->current_direction_batch_count = 0; // Reset batch count for new direction
        }
        manager->current_direction = next_direction;

        // Determine how many customers can go in this batch
        int batch_size = 0;
        if (manager->current_direction == DIRECTION_UP)
        {
            batch_size = queue_size(manager->waiting_to_go_up_customers);
        }
        else if (manager->current_direction == DIRECTION_DOWN)
        {
            batch_size = queue_size(manager->waiting_to_go_down_customers);
        }
        if (batch_size > manager->config->max_batch_size)
        {
            batch_size = manager->config->max_batch_size;
        }
        manager->current_patch_not_on_stairs_yet_count = batch_size;

        if (batch_size > 0)
        {
            print(manager, "Tick %d: New batch direction %s with batch size %d\n", tick,
                  manager->current_direction == DIRECTION_UP ? "UP" : "DOWN", batch_size);
            ++manager->current_direction_batch_count;
        }
    }

    // Waiting customers can start going if there's batch capacity
    if (manager->current_patch_not_on_stairs_yet_count > 0)
    {
        // Move customers from waiting to on stairs based on the current direction
        Customer *customer = NULL;
        if (manager->current_direction == DIRECTION_UP && !queue_is_empty(manager->waiting_to_go_up_customers))
        {
            customer = queue_dequeue(manager->waiting_to_go_up_customers);
            queue_enqueue(manager->on_stairs_customers, customer);
        }
        else if (manager->current_direction == DIRECTION_DOWN && !queue_is_empty(manager->waiting_to_go_down_customers))
        {
            customer = queue_dequeue(manager->waiting_to_go_down_customers);
            queue_enqueue(manager->on_stairs_customers, customer);
        }
        if (customer)
        {
            customer->current_step = 0;
            print(manager, "Tick %d: Customer %s starts going %s\n", tick, customer->name,
                  customer->direction == DIRECTION_UP ? "UP" : "DOWN");
            --manager->current_patch_not_on_stairs_yet_count;
        }
    }

    // Move customers on stairs one step closer to completion
    int total_stair_steps = manager->config->total_stair_steps;
    Queue *remaining_on_stairs_customers = queue_create();
    while (!queue_is_empty(manager->on_stairs_customers))
    {
        Customer *customer = queue_dequeue(manager->on_stairs_customers);
        if (customer->response_time == -1)
        {
            customer->response_time = tick - customer->arrival_time; // Set response time when they first step on stairs
        }

        if (customer->current_step < total_stair_steps)
        {
            customer->current_step += 1;                            // Move one step
            queue_enqueue(remaining_on_stairs_customers, customer); // Still on stairs
        }
        else
        {
            print(manager, "Tick %d: Customer %s finishes going %s\n", tick, customer->name,
                  customer->direction == DIRECTION_UP ? "UP" : "DOWN");
            // Customer finished, do not re-enqueue
            customer->turnaround_time = tick - customer->arrival_time; // Finalize turnaround time when they finish
        }
    }
    queue_destroy(manager->on_stairs_customers);
    manager->on_stairs_customers = remaining_on_stairs_customers;

    // If no customers can go, set direction to NONE
    if (queue_is_empty(manager->on_stairs_customers))
    {
        manager->current_direction = DIRECTION_NONE;
    }
}

// Custom comparator for qsort to sort customers by arrival time @Qingzheng
int sort_customers_by_arrival_time_comparator(const void *a, const void *b)
{
    const Customer *customer_a = *(const Customer **)a;
    const Customer *customer_b = *(const Customer **)b;
    return customer_a->arrival_time - customer_b->arrival_time;
}

// Sort customers by arrival time and return an array of pointers to the sorted customers @Qingzheng
Customer **sort_customers_by_arrival_time(Customer *customers, int total_customers)
{
    Customer **sorted_customers = (Customer **)malloc(sizeof(Customer *) * total_customers);
    for (int i = 0; i < total_customers; ++i)
    {
        sorted_customers[i] = &customers[i];
    }
    // C standard library qsort with custom comparator to sort by arrival time
    qsort(sorted_customers, total_customers, sizeof(Customer *), sort_customers_by_arrival_time_comparator);
    return sorted_customers;
}

// ============ @Yujie
// Helper func
// Return current time in microseconds (monotonic clock)
// 返回当前时间（微秒），单调时钟不会倒退
static long long now_usec()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;
}

// Convert wall time to "tick" where 1 tick = unit_usec
// 把真实时间换算成 tick：1 tick = unit_usec 微秒
static int now_tick(ThreadManager *manager)
{
    long long elapsed = now_usec() - manager->start_time;
    if (manager->config->unit_usec <= 0)
        return 0;
    return (int)(elapsed / (long long)manager->config->unit_usec);
}
// ================================

// @Yujie =======================
// Thread argument wrapper
typedef struct
{
    ThreadManager *manager;
    Customer *customer;
} CustomerThreadArg;

// Decide next direction when stairs are empty
// 楼梯空时决定下一批方向（防饥饿：连续批次到上限就换向）
static Direction choose_next_direction(ThreadManager *manager)
{
    // nobody waiting
    if (manager->waiting_up_count == 0 && manager->waiting_down_count == 0)
    {
        return DIRECTION_NONE;
    }
    // only one side waiting
    if (manager->waiting_up_count == 0)
        return DIRECTION_DOWN;
    if (manager->waiting_down_count == 0)
        return DIRECTION_UP;

    // starvation prevention: too many consecutive batches in same direction -> force switch
    if (manager->current_direction != DIRECTION_NONE &&
        manager->current_direction_batch_count >= manager->config->max_consecutive_batches)
    {
        return (manager->current_direction == DIRECTION_UP) ? DIRECTION_DOWN : DIRECTION_UP;
    }

    // otherwise, pick something (random / or larger waiting side)
    // 否则可以随机，或选等待人数更多的一侧（更“efficient”）
    if (manager->waiting_up_count > manager->waiting_down_count)
        return DIRECTION_UP;
    if (manager->waiting_down_count > manager->waiting_up_count)
        return DIRECTION_DOWN;

    return (rand() % 2 == 0) ? DIRECTION_UP : DIRECTION_DOWN;
}

// 启动新 batch（设置方向 + batch size + batch count）
static void start_new_batch_if_needed(ThreadManager *manager)
{
    // only do this when stairs are empty
    if (manager->on_stairs_count != 0)
        return;

    // if current batch still has remaining slots, keep it
    if (manager->current_patch_not_on_stairs_yet_count > 0 &&
        manager->current_direction != DIRECTION_NONE)
        return;

    Direction next = choose_next_direction(manager);

    if (next == DIRECTION_NONE)
    {
        manager->current_direction = DIRECTION_NONE;
        manager->current_patch_not_on_stairs_yet_count = 0;
        return;
    }

    // reset consecutive count if direction changes
    if (manager->current_direction != next)
    {
        manager->current_direction_batch_count = 0;
    }

    manager->current_direction = next;

    int waiting = (next == DIRECTION_UP) ? manager->waiting_up_count : manager->waiting_down_count;
    int batch_size = waiting;
    if (batch_size > manager->config->max_batch_size)
        batch_size = manager->config->max_batch_size;

    manager->current_patch_not_on_stairs_yet_count = batch_size;
    if (batch_size > 0)
        manager->current_direction_batch_count += 1;
}

// Customer thread function
// One customer: one thread
static void *customer_thread(void *arg)
{
    CustomerThreadArg *a = (CustomerThreadArg *)arg;
    ThreadManager *manager = a->manager;
    Customer *customer = a->customer;

    // 1. Simulate arrival time (tick → real time)
    // arrival_time 是 tick 单位，需要乘 unit_usec 才是实际时间
    usleep((useconds_t)((long long)customer->arrival_time * manager->config->unit_usec));

    print(manager,
          "Tick %d: Customer %s arrives waiting to go %s\n",
          customer->arrival_time,
          customer->name,
          customer->direction == DIRECTION_UP ? "UP" : "DOWN");

    // 2. Enter the waiting queue (Margin area)
    pthread_mutex_lock(&manager->mutex);

    if (customer->direction == DIRECTION_UP)
        manager->waiting_up_count++;
    else
        manager->waiting_down_count++;

    // 3. Wait until being allowed to enter the stairs
    // while (
    //     manager->current_direction != DIRECTION_NONE &&
    //     manager->current_direction != customer->direction)
    // {
    //     pthread_cond_wait(
    //         &manager->stairs_state_changed,
    //         &manager->mutex);
    // }

    // // 4. Can enter the stair
    // manager->current_direction = customer->direction;

    // 完全在 mutex 保护内
    pthread_mutex_lock(&manager->mutex);

    // 先登记等待人数（你原本有，保留）
    if (customer->direction == DIRECTION_UP)
        manager->waiting_up_count++;
    else
        manager->waiting_down_count++;

    // 核心：循环等待直到“能进入”
    // 条件 = (楼梯空时先启动新batch) + (方向匹配) + (本批还有名额)
    while (1)
    {
        // 如果楼梯空，可能需要启动新批次（决定方向+batch size）
        start_new_batch_if_needed(manager);

        int direction_ok = (manager->current_direction == customer->direction);
        int batch_ok = (manager->current_patch_not_on_stairs_yet_count > 0);

        // 能进入：方向一致 + batch还有名额
        if (direction_ok && batch_ok)
            break;

        pthread_cond_wait(&manager->stairs_state_changed, &manager->mutex);
    }

    // 允许进入：更新共享状态
    if (customer->direction == DIRECTION_UP)
        manager->waiting_up_count--;
    else
        manager->waiting_down_count--;

    manager->on_stairs_count++;
    manager->current_patch_not_on_stairs_yet_count--; // 本批名额减少

    pthread_mutex_unlock(&manager->mutex);

    if (customer->direction == DIRECTION_UP)
        manager->waiting_up_count--;
    else
        manager->waiting_down_count--;

    manager->on_stairs_count++;

    print(manager,
          "Customer %s STARTS going %s\n",
          customer->name,
          customer->direction == DIRECTION_UP ? "UP" : "DOWN");

    pthread_mutex_unlock(&manager->mutex);

    // Climb stairs（ semaphore control each stair）
    int S = manager->config->total_stair_steps;

    for (int i = 0; i < S; i++)
    {
        sem_wait(&manager->step_semaphores[i]);

        // 记录每个人走到哪一阶
        customer->current_step = i + 1;

        // ✅ 第一次踏上楼梯：记录 response_time
        // record response time at first step on stairs
        if (i == 0 && customer->response_time == -1)
        {
            int t = now_tick(manager);
            customer->response_time = t - customer->arrival_time;
        }

        if (i > 0)
            sem_post(&manager->step_semaphores[i - 1]);

        // ✅ 用配置的 unit_usec（别写死 100000）,你现在写死 usleep(100000) 会导致你输入的 unit_usec 参数完全没用。
        usleep(manager->config->unit_usec);
    }

    // release last stair
    if (S > 0)
    {
        sem_post(&manager->step_semaphores[S - 1]);
    }

    // ✅ 走完楼梯：记录 turnaround_time
    // record turnaround time when finished
    if (customer->turnaround_time == -1)
    {
        int t = now_tick(manager);
        customer->turnaround_time = t - customer->arrival_time;
    }

    // leave the stairs
    pthread_mutex_lock(&manager->mutex);

    manager->on_stairs_count--;

    print(manager,
          "Customer %s FINISHED going %s\n",
          customer->name,
          customer->direction == DIRECTION_UP ? "UP" : "DOWN");

    // 7. If stair is emtpy, reset direction

    if (manager->on_stairs_count == 0)
    {
        // batch用完 or 让系统重新选下一批
        // 楼梯空了就让下一次进入者触发新 batch
        // 这样下一波等待线程醒来时，start_new_batch_if_needed() 会根据等待人数 + consecutive 限制来选方向，保证不会饥饿。
        manager->current_patch_not_on_stairs_yet_count = 0;
        manager->current_direction = DIRECTION_NONE;
    }

    // 8. Wake up all waiting thread
    pthread_cond_broadcast(&manager->stairs_state_changed);

    pthread_mutex_unlock(&manager->mutex);

    return NULL;
}
// ==============================================

// [Exposed]
// Main runner @Qingzheng
void thread_manager_run(ThreadManager *manager, Customer *customers)
{
    if (!manager || !customers)
    {
        return;
    }

    // Sort customers by arrival time, then make it a queue for simulation loop
    int total_customers = manager->config->total_customers;
    for (int i = 0; i < total_customers; ++i)
    {
        customers[i].turnaround_time = -1; // Initialize turnaround time
        customers[i].response_time = -1;   // Initialize response time
        customers[i].current_step = 0;
    }

    // 2) Keep sorting if we want deterministic thread creation order
    //    (保留排序：线程创建顺序更稳定，arrival_time 还是由线程自己 sleep 控制)
    Customer **sorted_customers = sort_customers_by_arrival_time(customers, total_customers);

    // ====== @Yujie: Comment out
    //  Queue* incoming_customers = queue_create();
    //  for (int i = 0; i < total_customers; ++i) {
    //      queue_enqueue(incoming_customers, sorted_customers[i]);
    //  }

    // Simulation loop
    // int tick = 0;
    // while (true) {
    //     simulate(manager, incoming_customers, tick);

    //     if (queue_is_empty(incoming_customers) && queue_is_empty(manager->on_stairs_customers)
    //             && queue_is_empty(manager->waiting_to_go_up_customers) && queue_is_empty(manager->waiting_to_go_down_customers)) {
    //         break; // End simulation when no more customers are incoming or on the stairs
    //     }
    //     ++tick;
    // }
    // =====================================

    // Print final stats
    // print(manager, "Simulation finished at tick %d\n", tick);
    // int total_turnaround_time = 0, total_response_time = 0;
    // for (int i = 0; i < total_customers; ++i) {
    //     Customer* customer = &customers[i];
    //     print(manager, "Customer %s: turnaround time = %d, response time = %d\n", customer->name, customer->turnaround_time, customer->response_time);
    //     total_turnaround_time += customer->turnaround_time;
    //     total_response_time += customer->response_time;
    // }
    // print(manager, "Average turnaround time = %.3f\n", (double)total_turnaround_time / total_customers);
    // print(manager, "Average response time = %.3f\n", (double)total_response_time / total_customers);

    // // Destroy
    // queue_destroy(incoming_customers);
    // free(sorted_customers);
    // =======================================

    // ============================== @Yujie
    // Thread mode: we don't need incoming_customers queue + simulate tick loop
    // Queue* incoming_customers = queue_create();
    // for (...) queue_enqueue(...)

    // 3) Create one thread per customer (核心替换点：创建线程)
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * total_customers);
    CustomerThreadArg *args = (CustomerThreadArg *)malloc(sizeof(CustomerThreadArg) * total_customers);

    for (int i = 0; i < total_customers; ++i)
    {
        args[i].manager = manager;
        args[i].customer = sorted_customers[i]; // 注意：用排序后的 customer 指针

        // 每个 customer 一个线程
        pthread_create(&threads[i], NULL, customer_thread, &args[i]);
    }

    // 4) Join all threads, wait after all customer threads ends
    for (int i = 0; i < total_customers; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    // 5) Print original final stats
    // 注意：线程版本没有 tick loop，所以这里的 tick 没意义了
    // 你可以打印 "Simulation finished" 或者用最大完成时间替代
    print(manager, "Simulation finished (threaded)\n");

    int total_turnaround_time = 0, total_response_time = 0;
    for (int i = 0; i < total_customers; ++i)
    {
        Customer *customer = &customers[i];
        print(manager, "Customer %s: turnaround time = %d, response time = %d\n",
              customer->name, customer->turnaround_time, customer->response_time);
        total_turnaround_time += customer->turnaround_time;
        total_response_time += customer->response_time;
    }
    print(manager, "Average turnaround time = %.3f\n", (double)total_turnaround_time / total_customers);
    print(manager, "Average response time = %.3f\n", (double)total_response_time / total_customers);

    // 6) Destroy/Free (保留清理，但删掉 incoming_customers 那套)
    free(args);
    free(threads);

    free(sorted_customers);
    // =====================================
}
