#include "scheduling_simulator.h"

ucontext_t Shell;
ucontext_t Simulator;
ucontext_t Finish;
struct itimerval stop;
struct itimerval tick;
struct itimerval tick2;
Task* pidlisthead = NULL, *pidlisttail = NULL;
Task* Hlisthead = NULL, *Hlisttail = NULL;
Task* Llisthead = NULL, *Llisttail = NULL;
Task* newtask = NULL, *nowtask = NULL, *prevnowtask = NULL;
int pidcount = 0;
int usercreate = 0;


void hw_suspend(int msec_10)
{
    setitimer(ITIMER_REAL, &stop, NULL);
    // puts("suspend.");
    Task* nexttask;
    Task* suspendtask = nowtask;
    if(nowtask->priority=='H') {
        nexttask = Hlisthead;
        prevnowtask = Hlisthead;
    } else {
        nexttask = Llisthead;
        prevnowtask = Llisthead;
    }
    suspendtask->state = TASK_WAITING;
    suspendtask->suspendtime = msec_10 * 10;
    while(nexttask->schedulenext != NULL) {
        prevnowtask = nexttask;
        nexttask = nexttask->schedulenext;
        if(nexttask->state == TASK_READY) {  //find next ready task
            nowtask = nexttask;  //next
            nexttask->state = TASK_RUNNING;
            if(nexttask->tq==10) {
                setitimer(ITIMER_REAL, &tick, NULL);
            } else {
                setitimer(ITIMER_REAL, &tick2, NULL);
            }
            // printf("switch from %d %s", now->pid, now->name);
            // printf(" to %d %s\n", nowtask->pid, nowtask->name);
            swapcontext(&(suspendtask->context), &(nexttask->context));
            return;
        }
    }
    puts("all not ready.");	//if no ready task at all
    puts("waiting...");
    swapcontext(&(suspendtask->context), &Simulator);   //now:WAITING
    return;
}

void hw_wakeup_pid(int pid)
{
    Task* itr = Hlisthead;
    while(itr!=NULL) {
        if(itr->pid==pid && itr->state == TASK_WAITING) {
            itr->state = TASK_READY;
            break;
        }
        itr=itr->schedulenext;
    }
    if(itr == NULL) {
        itr = Llisthead;
        while(itr!=NULL) {
            if(itr->pid==pid && itr->state == TASK_WAITING) {
                itr->state = TASK_READY;
                break;
            }
            itr=itr->schedulenext;
        }
    }
    return;
}

int hw_wakeup_taskname(char *task_name)
{
    int count = 0;
    Task* itr = Hlisthead;
    while(itr!=NULL) {
        if(strcmp(itr->taskname,task_name)==0 && itr->state == TASK_WAITING) {
            itr->state = TASK_READY;
            ++count;
        }
        itr=itr->schedulenext;
    }
    itr = Llisthead;
    while(itr!=NULL) {
        if(strcmp(itr->taskname,task_name)==0 && itr->state == TASK_WAITING) {
            itr->state = TASK_READY;
            ++count;
        }
        itr=itr->schedulenext;
    }
    return count;
}

int hw_task_create(char *task_name)
{
    // puts(task_name);
    newtask = (Task*)malloc(sizeof(Task));
    if(strcmp(task_name, "task1") == 0) {
        newtask->func = task1;
    } else if(strcmp(task_name, "task2") == 0) {
        newtask->func = task2;
    } else if(strcmp(task_name, "task3") == 0) {
        newtask->func = task3;
    } else if(strcmp(task_name, "task4") == 0) {
        newtask->func = task4;
    } else if(strcmp(task_name, "task5") == 0) {
        newtask->func = task5;
    } else if(strcmp(task_name, "task6") == 0) {
        newtask->func = task6;
    } else {
        return -1;
    }

    newtask->schedulenext = NULL;
    newtask->state = TASK_READY;
    ++pidcount;
    newtask->pid = pidcount;
    newtask->tq = 10;
    newtask->queueingtime = 0;
    newtask->suspendtime = 0;
    strcpy(newtask->taskname,task_name);

    if(usercreate) return pidcount;
// puts("a");
    addToList(0);

    return pidcount; // the pid of created task name
}

void addToList(int priority)
{
    getcontext(&(newtask->context));
    newtask->context.uc_link = &Finish;	//if terminate before time quantum
    newtask->context.uc_stack.ss_sp = malloc(4096);
    newtask->context.uc_stack.ss_size = 4096;
    makecontext((&newtask->context), (void *)newtask->func, 0);

    if( priority == 1 ) { // H
        newtask->priority = 'H';
        if(Hlisthead == NULL)  //add to Hlist
            Hlisthead = newtask;
        if(Hlisttail == NULL)
            Hlisttail = newtask;
        else {
            Hlisttail->schedulenext = newtask;
            Hlisttail = newtask;
        }
    } else { // L
        newtask->priority = 'L';
        if(Llisthead == NULL) { //add to Llist
            Llisthead = newtask;
            puts("Lhead");
        }
        if(Llisttail == NULL)
            Llisttail = newtask;
        else {
            Llisttail->schedulenext = newtask;
            Llisttail = newtask;
        }
    }
    if(pidlisthead == NULL)  //add to Hlist
        pidlisthead = newtask;
    if(pidlisttail == NULL)
        pidlisttail = newtask;
    else {
        pidlisttail->pidnext = newtask;
        pidlisttail = newtask;
    }

    // puts("SS");
    // setcontext(&(newtask->context));
}

void simulator()
{
    // puts("[simu mode]");

    if(Hlisthead == NULL && Llisthead==NULL) {	//empty
        puts("queue is empty.");
        setcontext(&Shell);
        return;
    }
    // puts("z");
    if(nowtask!=NULL && nowtask->state == TASK_RUNNING) {    //if paused continue run it
        // puts("continue run paused");
        if(nowtask->tq==10) {
            setitimer(ITIMER_REAL, &tick, NULL);
        } else {
            setitimer(ITIMER_REAL, &tick2, NULL);
        }
        setcontext(&(nowtask->context));
        return;
    }
    // puts("a");
    Task* itr = Hlisthead;
    prevnowtask = Hlisthead;
    while(itr != NULL) {
        // puts("aa");
        prevnowtask = itr;
        if(itr->state == TASK_READY) {  //find the first ready task and run it
            itr->state = TASK_RUNNING;
            nowtask = itr;
            printf("pid %d %s in H ready->run",nowtask->pid,nowtask->taskname);
            if(nowtask->tq==10) {
                setitimer(ITIMER_REAL, &tick, NULL);
            } else {
                setitimer(ITIMER_REAL, &tick2, NULL);
            }
            // printf("run %d %s\n", itr->pid, itr->name);
            setcontext(&(itr->context));
            return;
        }
        itr = itr->schedulenext;
    }
    // puts("b");
    itr = Llisthead;
    prevnowtask = Llisthead;
    while(itr != NULL) {
        // puts("bb");
        prevnowtask = itr;
        if(itr->state == TASK_READY) {  //find the first ready task and run it
            itr->state = TASK_RUNNING;
            nowtask = itr;
            printf("pid %d %s in H ready->run\n",nowtask->pid,nowtask->taskname);
            if(nowtask->tq==10) {
                setitimer(ITIMER_REAL, &tick, NULL);
            } else {
                setitimer(ITIMER_REAL, &tick2, NULL);
            }
            // puts("bla");
            // printf("run %d %s\n", itr->pid, itr->name);
            setcontext(&(itr->context));
            // puts("blabla");
            return;
        }
        itr = itr->schedulenext;
    }
    //all task not ready
    itr = Hlisthead;
    while(itr != NULL) {
        prevnowtask = itr;
        if(itr->state == TASK_WAITING) {    //found one waiting task
            nowtask = itr;
            puts("waiting...H");
            if(nowtask->tq==10) {
                setitimer(ITIMER_REAL, &tick, NULL);
            } else {
                setitimer(ITIMER_REAL, &tick2, NULL);
            }
            while(1);	//wait timer to decrease suspend time
        }
        itr = itr->schedulenext;
    }
    itr = Llisthead;
    while(itr != NULL) {
        prevnowtask = itr;
        if(itr->state == TASK_WAITING) {    //found one waiting task
            nowtask = itr;
            puts("waiting...L");
            if(nowtask->tq==10) {
                setitimer(ITIMER_REAL, &tick, NULL);
            } else {
                setitimer(ITIMER_REAL, &tick2, NULL);
            }
            while(1);	//wait timer to decrease suspend time
        }
        itr = itr->schedulenext;
    }
    puts("all done.");  //all task terminated
    setcontext(&Shell);
    return;
}

void shell()
{
    setitimer(ITIMER_REAL, &stop, NULL);
    puts("[shell mode]");
    char input[64] = "";
    usercreate = 1; //flag for priority setting
    while(fgets(input, 64, stdin) != NULL) {
        // fputs(input, stdout);
        if(strcmp(input,"start\n")==0) break;
        if(strcmp(input,"h\n")==0) {
            puts("Hlist");
            if(Hlisthead == NULL)puts("H null");
            for(Task* itr = Hlisthead; itr != NULL; itr = itr->schedulenext) {
                printf("%d %s ",itr->pid,itr->taskname);
                switch(itr->state) {
                case TASK_RUNNING:
                    printf("TASK_RUNNING ");
                    break;
                case TASK_READY:
                    printf("TASK_READY ");
                    break;
                case TASK_WAITING:
                    printf("TASK_WAITING ");
                    break;
                case TASK_TERMINATED:
                    printf("TASK_TERMINATED ");
                    break;
                default:
                    printf("???");
                }
                printf("%d %c ",itr->queueingtime, itr->priority);
                (itr->tq==20)?printf("L\n"):printf("S\n");
                // 1 task1 TASK_READY 50 H L
            }
            continue;
        }
        if(strcmp(input,"l\n")==0) {
            puts("Llist");
            if(Llisthead == NULL)puts("L null");
            for(Task* itr = Llisthead; itr != NULL; itr = itr->schedulenext) {
                printf("%d %s ",itr->pid,itr->taskname);
                switch(itr->state) {
                case TASK_RUNNING:
                    printf("TASK_RUNNING ");
                    break;
                case TASK_READY:
                    printf("TASK_READY ");
                    break;
                case TASK_WAITING:
                    printf("TASK_WAITING ");
                    break;
                case TASK_TERMINATED:
                    printf("TASK_TERMINATED ");
                    break;
                default:
                    printf("???");
                }
                printf("%d %c ",itr->queueingtime, itr->priority);
                (itr->tq==20)?printf("L\n"):printf("S\n");
                // 1 task1 TASK_READY 50 H L
            }
            continue;
        }
        if(strcmp(input,"ps\n")==0) {
            for(Task* itr = pidlisthead; itr != NULL; itr = itr->pidnext) {
                printf("%d %s ",itr->pid,itr->taskname);
                switch(itr->state) {
                case TASK_RUNNING:
                    printf("TASK_RUNNING ");
                    break;
                case TASK_READY:
                    printf("TASK_READY ");
                    break;
                case TASK_WAITING:
                    printf("TASK_WAITING ");
                    break;
                case TASK_TERMINATED:
                    printf("TASK_TERMINATED ");
                    break;
                default:
                    printf("???");
                }
                printf("%d %c ",itr->queueingtime, itr->priority);
                (itr->tq==20)?printf("L\n"):printf("S\n");
                // 1 task1 TASK_READY 50 H L
            }
            /*
            Task* itrH = Hlisthead;
            Task* itrL = Llisthead;
            for(int i = 1; i <= pidcount; ++i){
                if(itrH!=NULL && itrH->pid == i){
                    printf("%d %s ",i,itrH->taskname);
                    switch(itrH->state){
                        case TASK_RUNNING: printf("TASK_RUNNING ");break;
                        case TASK_READY: printf("TASK_READY ");break;
                        case TASK_WAITING: printf("TASK_WAITING ");break;
                        case TASK_TERMINATED: printf("TASK_TERMINATED ");break;
                        default:printf("???");
                    }
                    printf("%d H ",itrH->queueingtime);
                    (itrH->tq==20)?printf("L\n"):printf("S\n");
                    // 1 task1 TASK_READY 50 H L
                    itrH = itrH->schedulenext;
                }else if(itrL!=NULL && itrL->pid == i){
                    printf("%d %s ",i,itrL->taskname);
                    switch(itrL->state){
                        case TASK_RUNNING: printf("TASK_RUNNING ");break;
                        case TASK_READY: printf("TASK_READY ");break;
                        case TASK_WAITING: printf("TASK_WAITING ");break;
                        case TASK_TERMINATED: printf("TASK_TERMINATED ");break;
                        default:printf("???");
                    }
                    printf("%d L ",itrL->queueingtime);
                    (itrL->tq==20)?printf("L\n"):printf("S\n");
                    itrL = itrL->schedulenext;
                }
            }
            */
            continue;
        }
        char* remain = "";
        remain = strtok(input, " "); // add
        if(strcmp(remain,"add")==0) {
            remain = strtok(NULL, " \n"); // Task?
            // char taskbuff[10] = "";
            // strcat(taskbuff,remain);
            int pid = hw_task_create(remain);
            if(pid == -1) {
                puts("no such task");
                continue;
            }
            int priority = 0, largeTQflag = 0;
            while(remain != NULL) {
                remain = strtok(NULL, " "); // -t or -p or \n
                if(remain == NULL)break; // \n
                if(strcmp(remain,"-t")==0) { // t
                    remain = strtok(NULL, " \n"); // L or S
                    if(strcmp(remain,"L")==0) { // L
                        largeTQflag = 1;
                    }
                } else if(strcmp(remain,"-p")==0) { // p
                    remain = strtok(NULL, " \n"); // H or L
                    if(strcmp(remain,"H")==0) { // H
                        priority = 1;
                    } else {
                        priority = 0;
                    }
                }
            }
            addToList(priority);
            if(largeTQflag) {
                newtask->tq = 20;
                largeTQflag = 0;
            }
        } else if(strcmp(remain,"remove")==0) {
            remain = strtok(NULL, " \n"); // pid
            int pid = atoi(remain);
            Task* prev = Hlisthead;
            Task* itr = Hlisthead;
            while(itr!=NULL) {
                if(itr->pid==pid) {
                    itr->state = TASK_TERMINATED;
                    if(prev == itr) {
                        prev->schedulenext = NULL;
                        itr->schedulenext = NULL;
                    } else {
                        prev->schedulenext = itr->schedulenext;
                        itr->schedulenext = NULL;
                    }
                    break;
                }
                itr=itr->schedulenext;
            }
            if(itr == NULL) {
                prev = Llisthead;
                itr = Llisthead;
                while(itr!=NULL) {
                    if(itr->pid==pid) {
                        itr->state = TASK_TERMINATED;
                        if(prev == itr) {
                            prev->schedulenext = NULL;
                            itr->schedulenext = NULL;
                        } else {
                            prev->schedulenext = itr->schedulenext;
                            itr->schedulenext = NULL;
                        }
                        break;
                    }
                    itr=itr->schedulenext;
                }
            }
        }
    }
    usercreate = 0;
    setcontext(&Simulator);
}

void finish()
{
    setitimer(ITIMER_REAL, &stop, NULL);
    nowtask->state = TASK_TERMINATED;
    int pid = nowtask->pid;
    if(Hlisthead == nowtask)
        Hlisthead = nowtask->schedulenext;
    else if(Llisthead == nowtask)
        Llisthead = nowtask->schedulenext;
    else {
        Task* prev, *itr;
        if(nowtask->priority == 'H') {
            prev = Hlisthead;
            itr = Hlisthead;
        } else {
            prev = Llisthead;
            itr = Llisthead;
        }
        while(itr!=NULL) {
            prev = itr;
            itr = itr->schedulenext;
            if(itr->pid==pid) {
                prev->schedulenext = itr->schedulenext;
                itr->schedulenext = NULL;
                break;
            }
        }
    }
    setcontext(&Simulator);
    return;
}

void pause(int signal)
{
    setitimer(ITIMER_REAL, &stop, NULL);
    puts("\npaused.");

    if(nowtask->state == TASK_RUNNING) {	//pause now running task
        // printf("stop %d %s back to shell\n", nowtask->pid, nowtask->name);
        swapcontext(&(nowtask->context), &Shell);
        return;
    }
    //if(nowtask->state == TASK_WAITING || nowtask->state == TASK_READY)
    //if in waiting loop or just wake up, back to shell
    //printf("stop %d %s back to simulator\n", nowtask->pid, nowtask->name);
    setcontext(&Shell);
    return;
}

void ticktock(int signal)
{
    // puts("time up!");
    // printf("nowtask : %d %s %d\n", nowtask->pid, nowtask->name, nowtask->state);
    Task *itr = Hlisthead;
    while(itr!=NULL) {
        if(itr->state == TASK_READY) {  //add queueing time
            itr->queueingtime += nowtask->tq;
        }
        if(itr->state == TASK_WAITING) {    //decrease suspend time
            itr->suspendtime = itr->suspendtime - nowtask->tq;
            if(itr->suspendtime <= 0) {
                printf("pid %d %s woke up.\n", itr->pid, itr->taskname);
                itr->state = TASK_READY;
                itr->suspendtime = 0;
            }
        }
        itr = itr->schedulenext;
    }

    itr = Llisthead;
    while(itr!=NULL) {
        if(itr->state == TASK_READY) {  //add queueing time
            itr->queueingtime += nowtask->tq;
        }
        if(itr->state == TASK_WAITING) {    //decrease suspend time
            itr->suspendtime = itr->suspendtime - nowtask->tq;
            if(itr->suspendtime <= 0) {
                printf("%d %s woke up.\n", itr->pid, itr->taskname);
                itr->state = TASK_READY;
                itr->suspendtime = 0;
            }
        }
        itr = itr->schedulenext;
    }

    if(nowtask->state == TASK_RUNNING) {    //switch task
        if(nowtask->schedulenext != NULL) { //if not tail put to tail
            nowtask->state = TASK_READY;
            if(nowtask->priority=='H') {
                if(Hlisthead == nowtask)
                    Hlisthead = nowtask->schedulenext;
                if(prevnowtask == nowtask) {
                    prevnowtask = Hlisttail;
                } else {
                    prevnowtask->schedulenext = nowtask->schedulenext;
                }
                Hlisttail->schedulenext = nowtask;
                Hlisttail = nowtask;
                nowtask->schedulenext = NULL;
            } else {
                if(Llisthead == nowtask)
                    Llisthead = nowtask->schedulenext;
                if(prevnowtask == nowtask) {
                    prevnowtask = Llisttail;
                } else {
                    prevnowtask->schedulenext = nowtask->schedulenext;
                }
                Llisttail->schedulenext = nowtask;
                Llisttail = nowtask;
                nowtask->schedulenext = NULL;
            }
            printf("stop %d %s back to simulator\n", nowtask->pid, nowtask->taskname);
            swapcontext(&(nowtask->context), &Simulator);   //now:READY
            return;
        }
        swapcontext(&(nowtask->context), &Simulator);
        return;
    }
    //if in waiting loop or just wake up
    // if(nowtask->state == TASK_WAITING || nowtask->state == TASK_READY) {
    // 	//printf("stop %d %s back to simulator\n", nowtask->pid, nowtask->name;
    // 	setcontext(&Simulator);	//back to simulator
    // 	return;
    // }
    // puts("got to simu");
    setcontext(&Simulator);
}

int main()
{
    getcontext(&Shell);
    Shell.uc_link = 0;
    Shell.uc_stack.ss_sp = malloc(4096); // 设置新的堆栈
    Shell.uc_stack.ss_size = 4096;
    makecontext (&Shell, shell, 0);

    getcontext(&Simulator);
    Simulator.uc_link = 0;
    Simulator.uc_stack.ss_sp = malloc(4096);
    Simulator.uc_stack.ss_size = 4096;
    makecontext (&Simulator, simulator, 0);

    getcontext(&Finish);
    Finish.uc_link = 0;
    Finish.uc_stack.ss_sp = malloc(4096);
    Finish.uc_stack.ss_size = 4096;
    makecontext (&Finish, finish, 0);

    signal(SIGALRM, ticktock);
    memset(&stop, 0, sizeof(stop));
    stop.it_value.tv_sec = 0;
    stop.it_value.tv_usec = 00;
    stop.it_interval.tv_sec = 0;
    stop.it_interval.tv_usec = 0;

    memset(&tick, 0, sizeof(tick));
    tick.it_value.tv_sec = 0;
    tick.it_value.tv_usec = 10000;
    tick.it_interval.tv_sec = 0;
    tick.it_interval.tv_usec = 0;

    memset(&tick2, 0, sizeof(tick2));
    tick2.it_value.tv_sec = 0;
    tick2.it_value.tv_usec = 20000;
    tick2.it_interval.tv_sec = 0;
    tick2.it_interval.tv_usec = 0;

    signal(SIGTSTP, pause);

    setcontext(&Shell);

    return 0;
}
