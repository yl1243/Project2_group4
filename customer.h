// Group 4 Project 2

// Defines the Customer struct and Direction enum @Qingzheng

#ifndef PROJECT_2_CUSTOMER_H
#define PROJECT_2_CUSTOMER_H

typedef enum {
    DIRECTION_NONE = 0, // Stairs are cleared
    DIRECTION_UP = 1, // Some customers are going up
    DIRECTION_DOWN = -1 // Some customers are going down
} Direction;

typedef struct {
    char* name;
    Direction direction;
    int arrival_time;

    int current_step; // Track: current step customer is on (unit: steps)
    int turnaround_time; // Track: total time customer spends from arrival to completion (unit: steps)
    int response_time; // Track: time from arrival to first step on stairs (unit: steps)
} Customer;

#endif //PROJECT_2_CUSTOMER_H