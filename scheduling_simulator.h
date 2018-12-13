#ifndef SCHEDULING_SIMULATOR_H
#define SCHEDULING_SIMULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include "task.h"

enum TASK_STATE {
    TASK_RUNNING,
    TASK_READY,
    TASK_WAITING,
    TASK_TERMINATED
};

typedef struct Task {
    ucontext_t context;
    struct Task* pidnext;
    struct Task* schedulenext;
    int state;
    void* func;
    int pid;
    int tq;
    int queueingtime;
    int suspendtime;
    char priority;
    char taskname[10];
} Task;

void hw_suspend(int msec_10);
void hw_wakeup_pid(int pid);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);
void shell();
void simulator();
void finish();
void addToList(int priority);

#endif
