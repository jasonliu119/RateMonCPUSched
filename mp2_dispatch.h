#ifndef MP2_DISPATCH_H
#define MP2_DISPATCH_H

#include <linux/sched.h>
#include "mp2_list.h"

extern struct mp2_task_struct * running_task;
extern struct task_struct * dispatch_thread;
extern struct mp2_task_struct task_list;

int LOW_PRIORITY=0;
int HIGH_PRIORITY=MAX_USER_RT_PRIO-1;

//MAX_USER_RT_PRIO-1

/*
 * set the currently running task able to be preempted
 */
void set_old_running_task(void)
{
    struct mp2_task_struct * ret;

    if(running_task==NULL)
    {
        return;
    }else
    {
        //running_task->state=READY;
        printk(KERN_INFO "Dispatching_Thread: change running task of id: %d to READY \n",running_task->pid);

        ret=set_mp2_task_state(running_task,READY);
        if(ret==NULL) return;

        struct sched_param sparam;
        sparam.sched_priority=LOW_PRIORITY;
        sched_setscheduler(running_task->linux_task,SCHED_NORMAL,&sparam);


        set_task_state(running_task->linux_task,TASK_UNINTERRUPTIBLE);
    }
}

/*
 * set the new running task with the highest priority
 */

void set_new_running_task(struct mp2_task_struct * new_running_task)
{
    struct mp2_task_struct * ret;

    if(new_running_task==NULL)
    {
        return;
    }else
    {
        printk(KERN_INFO "Dispatching_Thread: new task should be :%d ",new_running_task->pid);
        //new_running_task->state=RUNNING;
        ret=set_mp2_task_state(new_running_task,RUNNING);
        running_task= ret;

        if(ret!=NULL)
        {
            struct sched_param sparam;
            sparam.sched_priority=HIGH_PRIORITY;
            sched_setscheduler(new_running_task->linux_task,SCHED_FIFO,&sparam);

            wake_up_process(new_running_task->linux_task);
        }



    }

}


/*
 * the function in the dispatching thread
 */
int mp2_dispatch_fn(void* data)
{
    struct mp2_task_struct * new_running_task;

    while(1)
    {
        if(kthread_should_stop()) return 1;

        set_current_state(TASK_UNINTERRUPTIBLE);

        printk(KERN_INFO "Dispatching Thread: Start! \n");

        set_old_running_task();

        new_running_task=get_next_task();

        set_new_running_task(new_running_task);

        //put the dispatching thread to sleep

        printk(KERN_INFO "Dispatching Thread: schedule()! \n");
        schedule();
    }

    return 0;

}



#endif

/*
 * this is the scheduler
 * dispatching thread triggering context switch
 * it will work after yield message or wakeup_time expires
 */
 /*void dispatch()
 {
    // Preempt the current running task
    if (running_task != NULL)
    {
        struct sched_param sparam;
        sparam.sched_priority = 0;
        sched_setscheduler(task, SCHED_NORMAL, &sparam);
        running_task->state = READY;
    }

    mp2_task_struct * task = get_next_task();
    if (task != NULL)
    {
        struct sched_param sparam;
        wake_up_process(task);
        sparam.sched_priority = MAX_USER_RT_PRIO - 1;
        sched_setscheduler(task, SCHED_FIFO, &sparam);
    }


 }*/
