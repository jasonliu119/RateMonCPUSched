/*
 * mp2_test.c - a simple test application registered in the module running factorial computation.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

int ROUND=3000;
int JOB_NUM=5;
int PERIOD_PRO_RATIO=5;

/*
 * register the process in the module
 * return pid
 */
int register_proc(int period, int comp_time)
{
    int pid = getpid();
    printf("register_proc: pid : %d \n",pid);
    char buf [128];
    sprintf(buf, "echo R,%d,%d,%d > /proc/mp2/status", pid, period, comp_time);
    system(buf);

    return pid;
}

/*
 * verify the process was admitted or not
 */
int verify(int pid)
{
    char pid_str[10];
    sprintf(pid_str, "%d", pid);
    strcat(pid_str, ":");
    FILE * file = fopen("/proc/mp2/status", "r");
    if (file != NULL)
    {
        char line[128];
        while (fgets(line, sizeof(line), file) != NULL) // read a line
        {
            char * pch;
            pch = strstr(line, pid_str);
            if (pch != NULL)
            {
                fclose(file);
                return 1;
            }

        }
    }
    fclose(file);
    return 0;
}


/*
 * send yield message to proc
 */
void yield(int pid)
{
    printf("Yield! \n \n");

    char buf [128];
    sprintf(buf, "echo Y,%d > /proc/mp2/status", pid);
    system(buf);
}

/*
 * de-register the process in the module
 */
void deregister(int pid)
{
    char buf [128];
    sprintf(buf, "echo D,%d > /proc/mp2/status", pid);
    system(buf);
}

/*
 * running factorial computation
 */
void do_job()
{
    int j;
    for (j = 1; j < ROUND; j++)
    {
        unsigned long sum = 1;
        int i;
        for (i = 1; i < 1000000; i++)
            sum *= i;
    }
}


int get_job_time()
{
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    do_job();
    gettimeofday(&t2, NULL);
    int job_time = 1000 * (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000;
    return job_time;
}

int main()
{

    struct timeval t1, t2;
    printf("Input a number representing the number of timers for factorial: ");
    scanf("%d",&ROUND);


    printf("Input a number representing the rate of period over processing: ");
    scanf("%d",&PERIOD_PRO_RATIO);

    printf("Input a number representing the number of repeating jobs: ");
    scanf("%d",&JOB_NUM);

    // estimate the job time
    int comp_time = get_job_time();
    int period = PERIOD_PRO_RATIO * comp_time;

    printf("cal job time: %d ms \n", comp_time);

    // register in the module

    int pid = register_proc(period, comp_time);

    printf("computation time: %d ms, period: %d ms \n", comp_time,period);

    // verify the process was admitted
    if (!verify(pid))
    {
        printf("process %d was not admitted!\n", pid);
        exit(1);
    }

    yield(pid);


    // real-time loop
    int i;
    for (i=0; i<JOB_NUM; i++){
        system("cat /proc/mp2/status");

        gettimeofday(&t1,NULL);
        printf("Start Time of this job with ID %d : %d sec \n",pid,t1.tv_sec);

        do_job();

        gettimeofday(&t2,NULL);
        printf("End Time of this job with ID %d : %d sec \n",pid,t2.tv_sec);

        yield(pid);

    }

    deregister(pid);

    // read the list of processes
    //system("cat /proc/mp2/status");
    printf("Test application ends.\n");

    return 0;
}
