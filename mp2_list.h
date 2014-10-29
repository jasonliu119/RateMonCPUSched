/*
 * mp2_list.h - linked list to store process status
 */

#ifndef MP2_LIST_H
#define MP2_LIST_H

#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/rwsem.h>

#include <linux/timer.h>

#include "mp2_given.h"

extern struct mp2_task_struct * running_task;

typedef enum {RUNNING, READY, SLEEPING} state_t;

/*
 * define mp2_task_struct
 */
struct mp2_task_struct
{
    int pid;

    struct task_struct * linux_task;

    int period;
    int comp_time;

    state_t state;

    struct timer_list wakeup_timer;

    struct list_head list;
};

struct mp2_task_struct task_list;

/*
 * use rw_semaphore to protect the list
 */
struct rw_semaphore list_rw_sem;

/*
 * Slab Allocator
 */
struct kmem_cache * task_struct_cachep;

/*
 * initialize the list
 */
void init_list(void)
{
    init_rwsem(&list_rw_sem);
    INIT_LIST_HEAD(&task_list.list);
}

 /*
  * admission control  SIGMA(Ci/Pi) <= 0.693
  */

 bool can_add(int new_period, int new_comp_time)
 {
    struct mp2_task_struct * task;
    int comp_time_sum=0,period_time_sum=0;

    down_read(&list_rw_sem);

    list_for_each_entry(task, &task_list.list, list)
    {
        comp_time_sum+=task->comp_time;
        period_time_sum+=task->period;
    }

    up_read(&list_rw_sem);

    comp_time_sum+=new_comp_time;
    period_time_sum+=new_period;

    printk(KERN_INFO "Processing Sum: %d , Period Sum: %d ",comp_time_sum,period_time_sum);

    if((1000*comp_time_sum)/period_time_sum<=693) return true;
    else
        return false;
 }

/*
 * when a new process registers,
 * add a new node into the list
 */
//int add(struct task_struct * linux_task, int pid, int period, int comp_time)
int add(int pid, int period, int comp_time)
{
    printk(KERN_INFO "add: Processing Admission Control!");

    //decrease the rw_semaphore as a writer
    bool can_add_value = can_add(period, comp_time);

    if(!can_add_value)
    {
        printk(KERN_INFO "add: Fail to pass the Admission Control");
        return -1;
    }else
    {
        printk(KERN_INFO "add: Secceed to pass the Admission Control");
    }

    down_write(&list_rw_sem);

    struct mp2_task_struct * new_task;
    //new_task = kmalloc(sizeof(struct mp2_task_struct), GFP_KERNEL);
    new_task = kmem_cache_alloc(task_struct_cachep,GFP_KERNEL);

    new_task->pid = pid;
    new_task->period = period;
    new_task->comp_time = comp_time;
    new_task->linux_task = find_task_by_pid(pid);
    new_task->state = SLEEPING;

    setup_timer(&new_task->wakeup_timer,wakeup_timer_callback,new_task->pid);


    INIT_LIST_HEAD(&new_task->list);
    list_add(&new_task->list, &task_list.list);

    printk(KERN_INFO "add a process with pid = %d, period = %d, computation time = %d\n", pid, period, comp_time);
    //increase the rw_semaphore as a writer
    up_write(&list_rw_sem);

    return 0;
}




/*
 * delete a node from the list
 */
void delete_one_mp2_task(int pid)
{
    down_write(&list_rw_sem);
    struct mp2_task_struct * task, * tmp;
    list_for_each_entry_safe(task, tmp, &task_list.list, list)
    {
        if (task->pid == pid)
        {

            if(running_task==task)
            {
                running_task=NULL;
            }

            del_timer(&task->wakeup_timer);

            list_del(&task->list);

            kmem_cache_free(task_struct_cachep, task);

            //kfree(task);

            printk(KERN_INFO "delete process %d successfully!\n", pid);
            break;
        }
    }
    up_write(&list_rw_sem);
}


/*
 * set a node from the list
 */
struct mp2_task_struct* find_list(int pid)
{
    down_read(&list_rw_sem);
    struct mp2_task_struct * task;
    list_for_each_entry(task, &task_list.list, list)
    {
        if (task->pid == pid)
        {
            up_read(&list_rw_sem);
            return task;

        }
    }


    up_read(&list_rw_sem);
    return NULL;


}

struct mp2_task_struct * set_mp2_task_state(struct mp2_task_struct * task, state_t new_state)
{
    if(task==NULL)
        return NULL;

    down_write(&list_rw_sem);

    task->state=new_state;

    up_write(&list_rw_sem);

    return task;
}

/*
 * find a node from the list with its pid
 */
struct mp2_task_struct * find_set_mp2_task_state(int pid, state_t new_state)
{
    down_write(&list_rw_sem);
    struct mp2_task_struct * task;
    task=NULL;

    list_for_each_entry(task, &task_list.list, list)
    {
        if (task->pid == pid)
        {

            task->state=new_state;

            printk(KERN_INFO "set: set state of a task %d successfully!\n", pid);
            break;
        }
    }

    up_write(&list_rw_sem);

    return task;
}

/*
 * find the task with READY state that has the highest priority
 * (i.e. the shortest period)
 */
 struct mp2_task_struct * get_next_task(void)
 {
    down_read(&list_rw_sem);

    struct mp2_task_struct * task, * next_task;
    next_task = NULL;
    int min_period = INT_MAX;

    list_for_each_entry(task, &task_list.list, list)
    {
        if (task->state == READY && task->period < min_period)
        {
            next_task = task;
            min_period = task->period;
        }
    }

    up_read(&list_rw_sem);

    return next_task;
 }



/*
 * update the CPU time of the node in the list
 * if the ret=get_cpu_time is not 0, which means an unvalid process,
 * delete that node
 */
/*void update_list()
{
    struct ProcessStatus * aProcStatus,*tmp;

    down_write(&list_rw_sem);

    list_for_each_entry_safe(aProcStatus,tmp,&process_list.list,list)
    {
        printk(KERN_INFO "traverse a list_head");

        int ret=get_cpu_use(aProcStatus->pid,&aProcStatus->cpu_time);
        //if ret!=0, delete the node
        if(ret)
        {
            list_del(&aProcStatus->list);
            kfree(aProcStatus);
        }
    }

    up_write(&list_rw_sem);
}*/

/*
 * traverse the list and write the information of each node into the char buffer
 */
int read_list(char * page)
{
    //decrease the rw_semaphore as a reader
    down_read(&list_rw_sem);

    int len, pageoff = 0, state_num;
    struct mp2_task_struct * task;
    list_for_each_entry(task, &task_list.list, list)
    {
        if(task->state==READY)
        {
            state_num=2;
        }else if(task->state==RUNNING)
        {
            state_num=3;
        }else
        {
            state_num=1;
        }
        printk(KERN_INFO "read_list: with pid = %d, period = %d, computation time = %d\n", task->pid, task->period, task->comp_time);
        len = sprintf(&page[pageoff],"%d:PID, Period: %d, Processing Time: %d, State: %d\n", task->pid, task->period, task->comp_time, state_num);
        pageoff += len;
    }

    //increase the rw_semaphore as a reader
    up_read(&list_rw_sem);

    return pageoff;
}

/*
 * delete the whole list
 */
void delete_list(void)
{
    down_write(&list_rw_sem);
    struct mp2_task_struct * task, * tmp;
    list_for_each_entry_safe(task, tmp, &task_list.list, list)
    {
        //kmem_cache_free(task_struct_cachep, task->linux_task);

        del_timer(&task->wakeup_timer);

        list_del(&task->list);

        //kfree(task);
        kmem_cache_free(task_struct_cachep, task);

    }
    up_write(&list_rw_sem);

    printk(KERN_INFO "delete_list: Delete list successfully. \n");
}

#endif
