#include "../include/i2cbus1_control.h"

#define TRIGGER_BIT 0x27
#define BUFFER_SIZE 2
#define RESOLUTION_12_BITS 4096
#define SELECT_SCALE 2

static int isTerminate = 0;

uint8_t buff_x[BUFFER_SIZE];
uint8_t buff_y[BUFFER_SIZE];
uint8_t buff_z[BUFFER_SIZE];

static int16_t xenH_prev;
static int16_t yenH_prev;
static int16_t zenH_prev;
static int16_t xenH_curr;
static int16_t yenH_curr;
static int16_t zenH_curr;

static long long count_x;
static long long count_y;
static long long count_z;
static double accSum_x;
static double accSum_y;
static double accSum_z;
static double avg_x[BUFFER_SIZE];
static double avg_y[BUFFER_SIZE];
static double avg_z[BUFFER_SIZE];


static pthread_t i2cbus1XenH_id;
static pthread_t i2cbus1YenH_id;
static pthread_t i2cbus1ZenH_id;

static pthread_mutex_t xenH_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t yenH_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t zenH_mutex = PTHREAD_MUTEX_INITIALIZER;


//Initiate private function
void* I2cbus1readXenH_thread();
void* I2cbus1readYenH_thread();
void* I2cbus1readZenH_thread();
int16_t I2cbus1_getRawData(int8_t rawL, int8_t rawH);
float I2cbus1_calculateGForce(int16_t rawData);
double I2cbus1_calculateAvg(long long count, double accSum, double prevAvg);
int I2cbus1_triggerSound(double prevAvg, double prevRaw, double currAvg, double currRaw);



/*
#########################
#        PUBLIC         #
#########################
*/

void I2cbus1Control_init(void)
{
    I2cbus1_init();
    I2cbus1Write_Reg1(TRIGGER_BIT);

    if(I2cbus1Read_Reg1() != TRIGGER_BIT) 
    {
        printf("ERROR: fail to switch power mode and enable sensor.\n");
        exit(EXIT_FAILURE);
    }

    //Initiate pthread
    pthread_create(&i2cbus1XenH_id, NULL, I2cbus1readXenH_thread, NULL);

    pthread_create(&i2cbus1YenH_id, NULL, I2cbus1readYenH_thread, NULL);

    pthread_create(&i2cbus1ZenH_id, NULL, I2cbus1readZenH_thread, NULL);

}

void I2cbus1Control_join(void)
{
    pthread_join(i2cbus1XenH_id, NULL);
    pthread_join(i2cbus1YenH_id, NULL);
    pthread_join(i2cbus1ZenH_id, NULL);
}


void I2cbus1Control_cleanup(void)
{
    xenH_prev = 0;
    yenH_prev = 0;
    zenH_prev = 0;
    xenH_curr = 0;
    yenH_curr = 0;
    zenH_curr = 0;
    
}

void I2cbusControl_terminate(void)
{
    isTerminate = 1;
}



/*
#########################
#       PRIVATE         #
#########################
*/


void* I2cbus1readXenH_thread()
{
    while(!isTerminate)
    {
        pthread_mutex_lock(&xenH_mutex);
        xenH_prev = xenH_curr;

        //Convert raw to G force value
        buff_x[0] = I2cbus1Read_OutXL();
        buff_x[1] = I2cbus1Read_OutXH();
        xenH_curr = I2cbus1_calculateGForce(I2cbus1_getRawData(buff_x[0], buff_x[1]));

        //Update count & accumulate sum
        count_x ++;
        accSum_x += xenH_curr;

        //Update previous average & current average
        avg_x[0] = avg_x[1];
        avg_x[1] = I2cbus1_calculateAvg(count_x, accSum_x, avg_x[0]);
        
        //Trigger sound
        if(I2cbus1_triggerSound(avg_x[0], xenH_prev, avg_x[1], xenH_curr))
        {
            AudioMixerControl_addDrum(1);
        }

        pthread_mutex_unlock(&xenH_mutex);
    }

    return NULL;
}


void* I2cbus1readYenH_thread()
{
    while(!isTerminate)
    {
        pthread_mutex_lock(&yenH_mutex);
        yenH_prev = yenH_curr;

        //Convert raw to G force value
        buff_y[0] = I2cbus1Read_OutYL();
        buff_y[1] = I2cbus1Read_OutYH();
        yenH_curr = I2cbus1_calculateGForce(I2cbus1_getRawData(buff_y[0], buff_y[1]));
        
        //Update count & accumulate sum
        count_y ++;
        accSum_y += yenH_curr;

        //Update previous average & current average
        avg_y[0] = avg_y[1];
        avg_y[1] = I2cbus1_calculateAvg(count_y, accSum_y, avg_y[0]);

        //Trigger the sound
        if(I2cbus1_triggerSound(avg_y[0], yenH_prev, avg_y[1], yenH_curr))
        {
            AudioMixerControl_addDrum(1);
        }
        
        pthread_mutex_unlock(&yenH_mutex);
    }

    return NULL;
}

void* I2cbus1readZenH_thread()
{
    while(!isTerminate)
    {
        pthread_mutex_lock(&zenH_mutex);
        zenH_prev = zenH_curr;

        //Convert raw data to G force
        buff_z[0] = I2cbus1Read_OutZL();
        buff_z[1] = I2cbus1Read_OutZH();
        zenH_curr = I2cbus1_calculateGForce(I2cbus1_getRawData(buff_z[0], buff_z[1]));

        //Update count & accumulate sum
        count_z ++;
        accSum_z += zenH_curr;

        //Update previous average & current average
        avg_z[0] = avg_z[1];
        avg_z[1] = I2cbus1_calculateAvg(count_z, accSum_z, avg_z[0]);

        //Trigger the sound
        if(I2cbus1_triggerSound(avg_z[0], zenH_prev, avg_z[1] ,zenH_curr))
        {
            AudioMixerControl_addDrum(2);
        }
        
        pthread_mutex_unlock(&zenH_mutex);
    }

    return NULL;
}

int16_t I2cbus1_getRawData(int8_t rawL, int8_t rawH)
{
    return (rawH << 8) | (rawL);
}

float I2cbus1_calculateGForce(int16_t rawData)
{
    return (float)rawData * SELECT_SCALE/RESOLUTION_12_BITS;
}

double I2cbus1_calculateAvg(long long count, double accSum, double prevAvg)
{
    //Update previous average - this is overall average - not tight to the batch
    if(count == 1){
        return regularAvg(count, accSum);
    }
    else{
        return exponentialAvg(regularAvg(count, accSum), prevAvg);   
    }
}

int I2cbus1_triggerSound(double prevAvg, double prevRaw, double currAvg, double currRaw)
{
    if((fabs(prevAvg - prevRaw) > 4) && (fabs(currAvg - currRaw) > 4))
    {
        return 1;
    }
    return 0;
}