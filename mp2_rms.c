#include "mp2_rms.h"
#include "mp2_list.h"
#include "mp2_dispatch.h"

static struct proc_dir_entry *my_proc_dir;
static struct proc_dir_entry *my_proc_file;

extern int LOW_PRIORITY;
extern int HIGH_PRIORITY;


// current task
struct mp2_task_struct * running_task;
struct task_struct * dispatch_thread;

spinlock_t spinlock;


/*
 * Callback function for the timer
 */
void wakeup_timer_callback(unsigned long pid)
{
    unsigned long flag;

    printk(KERN_INFO "wakeup_timer_callback: set to READY and wake up dispatching");

    //use spin_lock to protect shared valuable
    spin_lock_irqsave(&spinlock,flag);

    struct mp2_task_struct* this_task=find_list((int)pid);

    if(this_task!=NULL)
    {
        set_mp2_task_state(this_task,READY);
    }

    wake_up_process(dispatch_thread);

    spin_unlock_irqrestore(&spinlock,flag);
}

/*
 * module initialize function
 */
int mod_init(void)
{
    spin_lock_init(&spinlock);
    // slab allocation
    task_struct_cachep = kmem_cache_create("mp2_task_struct", sizeof(struct mp2_task_struct), ARCH_MIN_TASKALIGN, SLAB_PANIC | SLAB_NOTRACK, NULL);


    //initialize the task list
    init_list();
    printk(KERN_INFO "Init Linked List successfully \n");

    // create proc directory /proc/mp2
    my_proc_dir = proc_mkdir("mp2", NULL);
    if(!my_proc_dir)
    {
        printk(KERN_INFO "create dir fail\n");
        return -1;
    }
    else
        printk(KERN_INFO "Create /proc/mp2/ successfully \n");

    // create proc entry /proc/mp2/status
    my_proc_file = create_proc_entry("status", 0666, my_proc_dir);
    if(!my_proc_file)
    {
        printk("create proc entry fails \n");
        remove_proc_entry("mp2",NULL);
        return -1;
    }
    else
        printk(KERN_INFO "Create /proc/mp2/status successfully \n");

    // assign read and write callback function
    my_proc_file->write_proc = write_callback;
    my_proc_file->read_proc = read_callback;


    running_task = NULL;


    // create the dispatch thread
    dispatch_thread = kthread_create(mp2_dispatch_fn, NULL, "mp2_dispatch_thread"); // call the dispatch function
    if(!IS_ERR(dispatch_thread))
    {
        printk(KERN_INFO "Create dispatching thread successfully \n");
    }else
    {
        printk(KERN_INFO "Fail to create dispatching thread  \n");
    }

    //struct sched_param sparam;
    //sparam.sched_priority = HIGH_PRIORITY; // dispatch thread has the highest real time priority
    //sched_setscheduler(dispatch_thread, SCHED_FIFO, &sparam);

    return 0;
}


/*
 * module exit funtion
 */
void mod_exit(void)
{


    // stop the dispatching thread
    if(!IS_ERR(dispatch_thread))
    {
        kthread_stop(dispatch_thread);
        printk(KERN_INFO "mod_exit: Stop dispatch_thread successfully. \n");
    }

    //delete workqueue
    //rm_workqueue();

    //delete the list
    delete_list();


    kmem_cache_destroy(task_struct_cachep);

    remove_proc_entry("status", my_proc_dir);
    remove_proc_entry("mp2", NULL);
    printk(KERN_INFO "mod_exit: Delete proc entry successfully. \n");
}

/*
 * write callback function of /proc/mp2/status
 */
ssize_t write_callback(struct file * filp, const char __user *buff,
                               unsigned long len, void *data)
{
    char * kbuff;
    kbuff=(char *)vmalloc(MAX_BUFF_LENGTH);

    if(!kbuff)
        return -ENOMEM;
    else
        memset(kbuff,0,MAX_BUFF_LENGTH);

    if(len > MAX_BUFF_LENGTH)
    {
        printk(KERN_INFO "process_refister_write: write too long\n");
        return -ENOSPC;
    }

    if(copy_from_user(kbuff,buff,len))
        return -EFAULT;

    printk(KERN_INFO "kbuff: %s", kbuff);

    // parse and handle message
    message_handler(kbuff);

    vfree(kbuff);
    return len;
}

/*
 * Parse and handle message from /proc
 */
void message_handler(char * kbuff)
{
    switch (kbuff[0])
    {
        // registration message
        case 'R':
            register_handler(kbuff);
            break;

        // yield message
        case 'Y':
            yield_handler(kbuff);
            break;

        // de-registration message
        case 'D':
            deregister_handler(kbuff);
            break;

        default:
            printk(KERN_INFO "Message Error! \n");
            break;
    }
}

/*
 * handle registeration message
 * add a process to the list
 */
void register_handler(char * kbuff)
{
    int pid, period, comp_time;
    int re;
    //struct task_struct * linux_task;

    printk(KERN_INFO "Message: Registration\n");
    re = sscanf(kbuff,"R,%d,%d,%d",&pid,&period,&comp_time);
    if (re != 3)
    {
        printk(KERN_INFO "Registration message parsing error\n");
        return;
    }
    printk(KERN_INFO "Pid: %d, Period: %d, Computation Time: %d\n", pid, period, comp_time);


    //linux_task = kmem_cache_alloc(task_struct_cachep, GFP_KERNEL);

    add(pid, period, comp_time);

}

/*
 * handle yield message
 */
void yield_handler(char * kbuff)
{
    int pid;
    struct  mp2_task_struct * task;
    int re;
    struct sched_param sparam;

    printk(KERN_INFO "Message: Yield\n");
    re= sscanf(kbuff,"Y,%d",&pid);

    if (re != 1)
    {
        printk(KERN_INFO "Yield message parsing error\n");
    }
    else
    {
        printk(KERN_INFO "Pid: %d\n", pid);

        task = find_list(pid);

        if(task==NULL)
        {
            printk(KERN_INFO "yield_handler: no task with this pid is found in list");
        }else
        {
            if(running_task==task)
            {
                running_task=NULL;
            }



            printk(KERN_INFO "Find a task with Pid: %d \n", task->pid);
            set_mp2_task_state(task,SLEEPING);


            printk(KERN_INFO "Set a linux-task with Pid: %d to SLEEPING \n", pid);

            set_task_state(task->linux_task,TASK_UNINTERRUPTIBLE);

            sparam.sched_priority=LOW_PRIORITY;
            sched_setscheduler(task->linux_task,SCHED_NORMAL,&sparam);

            //schedule();
            printk(KERN_INFO "Set a timer of task Pid: %d with time: %d \n", pid,task->period-task->comp_time);

            mod_timer((&task->wakeup_timer),jiffies+msecs_to_jiffies(task->period-task->comp_time));



        }

        printk(KERN_INFO "Wake up dispatching thread: %d\n ", pid);
        wake_up_process(dispatch_thread);

    }

}




/*
 * handle de-registeration message
 * remove the process from the list
 */
void deregister_handler(char * kbuff)
{
    int pid;
    int re = sscanf(kbuff,"D,%d",&pid);

    printk(KERN_INFO "Message: De-registration\n");

    if (re != 1)
        printk(KERN_INFO "De-registration message parsing error\n");
    else
    {
        printk(KERN_INFO "Pid: %d\n", pid);
        delete_one_mp2_task(pid);
    }
}


/*
 * read callback function of /proc/mp2/status
 */
int read_callback(char *page,char ** start,off_t off,int count, int *eof,void *data)
{
    int len;

    printk(KERN_INFO "process list read callback\n");

    if(off > 0)
    {
        *eof = 1;
        return 0;
    }

    len = 0;
    len = read_list(page);
    return len;
}



module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CS423 MP2");
MODULE_AUTHOR("G3");

/*
void wakeup_timer_callback(unsigned long pid)
{
    //struct mp2_task_struct* task=(struct mp2_task_struct*)task_addr;

    struct mp2_work * my_work;
    my_work=(struct mp2_work*)kmalloc(sizeof(struct mp2_work),GFP_KERNEL);
    if(my_work)
    {
        //INIT_WORK((struct work_struct *)my_work,wakeup_timer_work_fn);
        INIT_WORK(&(my_work->work),wakeup_timer_work_fn);
        my_work->mp2_task=task;

        queue_work(wq,(struct work_struct *)my_work);
    }else
    {
        printk(KERN_INFO "wakeup_timer_callback: fail to kmalloc for work_t");
    }
    }*/

/*
struct workqueue_struct * wq;

struct mp2_work {
    struct work_struct work;
    struct mp2_task_struct* mp2_task;
};


void init_workqueue(void)
{
    wq=create_workqueue("my_queue");
    if(!wq)
    {
        printk(KERN_INFO "init_workqueue: Fail to create work_queue");
    }
}

void rm_workqueue(void)
{
    flush_workqueue(wq);
    destroy_workqueue(wq);
    printk(KERN_INFO "rm_workqueue: Succeed to remove work_queue");
}

void wakeup_timer_work_fn(struct work_struct * work)
{
    printk(KERN_INFO "Wakeup timer handler: set the task READY \n");
    //put the work_struct to the workqueue_struct
    struct mp2_work * my_work= (struct mp2_work *) work;

    if(my_work->mp2_task!=NULL)
    {
        set_mp2_task_state(my_work->mp2_task,READY);
    }

    //remember to free the space of the work
    kfree((void*)work);

    printk(KERN_INFO "Wakeup timer handler: wake up dispatch_thread \n");
    wake_up_process(dispatch_thread);

}*/
