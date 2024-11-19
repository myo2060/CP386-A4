#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>


// initialise semaphore
sem_t running;
sem_t even;
sem_t odd;
sem_t goofy;

//function to log that a new thread is started
void logStart(char * tID);

//function to log that a new thread is finished
void logFinish(char * tID);

//function to start program clock
void startClock();
//function to check current time since clock was started
long getCurrentTime();

time_t programClock; // the global timer/Clock for the program

//initialise global bools
bool f_thread = true;
bool t_started = false;

typedef struct thread // represents a single thread
{
    char tid[4];
    unsigned int startTime;
    int state;
    pthread_t handle;
    int retVal;
} Thread;

int threadsLeft(Thread *threads, int threadCount);
int threadToStart(Thread *threads, int threadCount);
void* threadRun(void *t);//the thread functuin, the code executed by each thread
int readFile(char *filename, Thread **threads); //function to read the file content and build array of threads


//main function
int main(int argc, char *argv[]){
    if (argc < 2){
        printf("Input file name missing.Exit with error code -1\n ");
        return -1;
    }

    //set semaphore values
    sem_init(&running, 0, 1);
    sem_init(&even, 0,0);
    sem_init(&odd, 0,0);
    sem_init(&goofy, 0,0);

    Thread *threads = NULL;
    int thread_Count = readFile(argv[1], &threads);
    startClock();

    //start looping
    while (threadsLeft(threads, thread_Count)>0){
        int i = 0; //initialise i to 0

        while((i = threadToStart(threads, thread_Count)) > -1){
            //if first thread true
            if (f_thread){
                f_thread = false;
                if (threads[i].tid[2] % 2 == 0)
                    sem_post(&even);
                else
                    sem_post(&odd);
            }
            //set state
            threads[i].state = 1;
            threads[i].retVal = pthread_create(&(threads[i].handle),NULL,
            threadRun, &threads[i]);
        }
    }

    //maintain t_started
    if (!t_started){
        int ready = true;
        for (int i = 0; i < thread_Count; i++)
            if (threads[i].state == 0)
                ready = false;

        if (ready)
            t_started = true;
    }

    //destroy semaphores
    sem_destroy(&running);
    sem_destroy (&even);
    sem_destroy (&odd);
    sem_destroy (&goofy);

    //close the program 
    return 0;

}

//Given read function
int readFile(char * fileName, Thread **threads){
    FILE *in = fopen(fileName, "r");
    if (!in){
        printf("Child A: Error in opening input file...exiting with error code -1\n");

        return -1;
    }
    struct stat st;
    fstat(fileno(in), &st);
    char *fileContent = (char*) malloc(((int) st.st_size + 1)* sizeof(char));
    fileContent[0] = '\0';
    while (!feof(in)) {
            char line[100];
            if (fgets(line, 100, in) != NULL) {
                    strncat(fileContent, line, strlen(line));
            }
    }
    fclose(in);

    char *command = NULL;
    int threadCount = 0;
    char *fileCopy = (char*) malloc((strlen(fileContent) + 1) * sizeof(char));
    strcpy(fileCopy, fileContent);
    command = strtok(fileCopy, "\r\n");
    while (command != NULL){
        threadCount++;
        command = strtok(NULL, "\r\n");
    }
    *threads = (Thread*) malloc(sizeof(Thread) * threadCount);

    char *lines[threadCount];
    command = NULL;
    int i = 0;
    command = strtok(fileContent, "\r\n");
    while (command != NULL) {
        lines[i] = malloc(sizeof(command) * sizeof(char));
        strcpy(lines[i], command);
        i++;
        command = strtok(NULL, "\r\n");
    }

    for (int k = 0; k < threadCount;k++){
        char *token = NULL;
        int j = 0;
        token = strtok(lines[k], ";");
        while (token != NULL) {
            (*threads)[k].state = 0;
            if (j == 0)
                  strcpy((*threads)[k].tid, token);

            if (j == 1)
                   (*threads)[k].startTime = atoi(token);
            j++;
            token = strtok(NULL, ";");
            }
     }
     return threadCount;
}

//helper function given| print when thread starts
void logStart(char *tID){
    printf("[%ld] New Thread with ID %s is started.\n", getCurrentTime(), tID);
}

void logFinish(char *tID){
    printf("[%ld] Thread with ID %s is finished.\n", getCurrentTime(), tID);
}

//helper function given | calculate remaining threads
int threadsLeft(Thread *threads, int threadCount) {
    int remainingThreads = 0;
    for (int k = 0; k < threadCount; k++) {
        if (threads[k].state > -1)
              remainingThreads++;

    }
    
    return remainingThreads;
}

//helper function given | calculate starting thread
int threadToStart(Thread *threads, int threadCount){
    for (int k = 0; k < threadCount; k++){
        if (threads[k].state == 0 && threads[k].startTime == getCurrentTime())
            return k;
    }
    return -1;
}

//Run threads through their critical sections

void* threadRun(void *t)
    {
          Thread *thread = (Thread *)t;
          logStart(((Thread*) t)->tid);
          //critical section starts
          if (!t_started) {
                if(thread->tid[2] == 49) //thread = t01
                {
                      sem_wait(&goofy);
                }
            else if (thread->tid[2] % 2){
                      sem_wait(&odd);
            } 
            else{
                      sem_wait(&even);
                }
            sem_wait(&running);
        }
        //print that thread is in crit section
        printf("[%ld] Thread %s is in its critical section\n", getCurrentTime(),
                    ((Thread*) t)->tid);
        //release critical section
        if (!t_started) {
            if(thread->tid[2] == 55){ //thread = t07
                    sem_post(&goofy);
            }
            if(thread->tid[2] == 49){ //thread = t01
                    sem_post(&odd);
            }
            else if(thread->tid[2] % 2){
                      sem_post(&even);
                      }
            else{
                      sem_post(&odd);
                }
            sem_post(&running);
          }
          else{
            int value;
            sem_getvalue(&even, &value);
            if (value == 0){
                      sem_post(&even);
                }
            sem_getvalue(&odd, &value);
            if (value == 0){
                      sem_post(&odd);
                }
            sem_post(&running);
        }
          //print and close thread.
          logFinish(((Thread*) t)->tid);
          ((Thread*) t)->state = -1;
          pthread_exit(0);
}

void startClock() {
    programClock = time(NULL);
}
    
long getCurrentTime() //invoke this method whenever you want check how much time units passed
//since you invoked startClock()
{
    time_t now;
    now = time(NULL);
    return now - programClock;
}




