RateMonCPUSched
===============
1. Overview
This project develops a Rate Monotonic Scheduler for Linux using Linux Kernel Modules. 
Round-base Admission Control and basic kernel API are used. Slab allocator is also utilized 
to improve the performance. We also implement a test application to test our real-time scheduler.

2. Compile and Test
mp2_rms.h/mp2_rms.c -- Give the kernel module implementation
mp2_list.h -- the list data structure and some functions, like change a state of a task, travel the list
mp2_dispatch.h -- dispatching thread to schedule
mp2_test.c -- test application
To compile the module and the test application, simply type “make”. To install the module, type “sudo insmod mp2_rms.ko”.
To unload the module, type “sudo rmmod mp2_rms”.
To test the module, you can run multiple test applications by typing “./mp2_test”, and run “cat /proc/mp2/status” to 
observe the process list. You can input different parameters in several test applications to see the result.
