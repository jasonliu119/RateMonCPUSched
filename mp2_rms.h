/*
 * mp2_rms module - Rate Monotonic Scheduler
 */

#ifndef MP2_RMS_H
#define MP2_RMS_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>



#define MAX_BUFF_LENGTH 256

/*
 * initialize the work_queue
 */
//void init_workqueue(void);

/* remove the work_queue
 *
 */
//void rm_workqueue(void);

/*
 * bottom-half handler: call the update_list() function to update the list
 */
//void wakeup_timer_work_fn(struct work_struct * work);

/*
 * wakeup_timer_callback function: update the timer
 * put work into workqueue
 */
void wakeup_timer_callback(unsigned long task_addr);

/*
 * module initialize function
 */
int mod_init(void);

/*
 * module exit funtion
 */
void mod_exit(void);

/*
 * write callback function of /proc/mp2/status
 */
ssize_t write_callback(struct file * filp, const char __user *buff,
                               unsigned long len, void *data);

/*
 * Parse and handle message from /proc
 */
void message_handler(char * kbuff);

/*
 * handle registeration message
 */
void register_handler(char * kbuff);

/*
 * handle yield message
 */
void yield_handler(char * kbuff);

/*
 * handle de-registeration message
 */
void deregister_handler(char * kbuff);

/*
 * read callback function of /proc/mp2/status
 */
int read_callback(char *page,char ** start,off_t off,int count, int *eof,void *data);

/*
 * initialize the work_queue
 */
// void init_workqueue();

/*
 * bottom-half handler: call the update_list() function to update the list
 */
// void work_func(struct work_struct * work);

/*
 * initialize the timer for updating the list
 */
//int init_timer();

/*
 * update_timer_callback function: update the timer
 * put work into workqueue
 */
// void timer_callback(unsigned long data);

#endif

