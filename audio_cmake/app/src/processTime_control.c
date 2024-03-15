#include "../include/processTime_control.h"

#define MAX_BUFFER_SIZE 300
#define MAX_SENDMSG_SIZE 800
#define MAX_TIME_PARTS 2

#define PROC_UPTIME_PATH "/proc/uptime"

//Statistic
static double stats_refillBuffer[3];
static double stats_accelerometer[3];
static int curr_mode;
static int curr_tempo;
static int curr_volume;

static int isTerminate;
static char buffer[MAX_BUFFER_SIZE];
static char* timeToParts[MAX_TIME_PARTS];
static char messageToSend[500];

static pthread_t processUpTime_id;

//Initate Private function
static void splitTimeToParts(char *input, char *intoParts[]);
void* processUpTime_thread();
void isAliveMessage(void);
void readFromProcessUptime(void);
void getStats_refillBuffer(void);
void getStats_accelerometer(void);


/*
#############################
#           PUBLIC          #
#############################
*/


void ProcessTime_init(void) 
{
    isTerminate = 0;

    //Create thread
    pthread_create(&processUpTime_id, NULL, processUpTime_thread, NULL);
}

void ProcessTime_join(void)
{
    pthread_join(processUpTime_id, NULL);
}

void ProcessTime_terminate(void)
{
    isTerminate = 1;
}

void ProcessTime_cleanup(void)
{
    //Clean data before use
    memset(buffer, 0, sizeof(buffer));
    memset(messageToSend, 0, sizeof(messageToSend));
    for (int i = 0; i < MAX_TIME_PARTS; i++) {
        timeToParts[i] = NULL;
    }
}


/*
#############################
#          PRIVATE          #
#############################
*/

void getCurrentMode(void) 
{
    curr_mode = AudioMixerControl_getMode();   
}

void getStats_refillBuffer(void)
{
    AudioMixerControl_getStats(&stats_refillBuffer[0], &stats_refillBuffer[1], &stats_refillBuffer[2]);
}

void getStats_accelerometer(void)
{
    I2cbusControl_getStats(&stats_accelerometer[0], &stats_accelerometer[1], &stats_accelerometer[2]);
}


void* processUpTime_thread(void)
{
    while(!isTerminate) 
    {
        readFromProcessUptime();
        UDP_sendToTarget(messageToSend);

        isAliveMessage();
        UDP_sendToTarget(messageToSend);
        
        //Sleep for 1 second
        sleepForMs(1000);
    }

    return NULL;
}

void isAliveMessage(void)
{
    memset(messageToSend, 0, sizeof(messageToSend));
    snprintf(messageToSend, MAX_SENDMSG_SIZE, "show_error,hide");
}

void readFromProcessUptime(void)
{
    //Clean data before use
    memset(buffer, 0, sizeof(buffer));
    memset(messageToSend, 0, sizeof(messageToSend));
    for (int i = 0; i < MAX_TIME_PARTS; i++) {
        timeToParts[i] = NULL;
    }

    //Get time from /proc/uptime
    readFromFileToBuffer(PROC_UPTIME_PATH, buffer, MAX_BUFFER_SIZE);
    splitTimeToParts(buffer, timeToParts);
    snprintf(messageToSend, MAX_SENDMSG_SIZE, "timer,%s", timeToParts[0]);
}


static void splitTimeToParts(char *input, char *intoParts[]) 
{
    char *token;
    int partNum = 0;

    // Get the first token
    token = strtok(input, " ");

    // Continue getting tokens until NULL or 3 parts are found
    while (token != NULL && partNum < MAX_TIME_PARTS) {
        intoParts[partNum] = token;
        token = strtok(NULL, " ");
        partNum++;
    }
}

