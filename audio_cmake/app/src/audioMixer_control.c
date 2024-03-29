#include "../include/audioMixer_control.h"
#include "../include/audioMixer_template.h"
#include "../include/audioMixer_helper.h"

#define MAX_STD_BEAT 3

#define STDBEAT_DRUM "/mnt/remote/myApps/beatbox-wav-files/100051__menegass__gui-drum-bd-hard.wav"
#define STDBEAT_HIT_HAT "/mnt/remote/myApps/beatbox-wav-files/100053__menegass__gui-drum-cc.wav"
#define STDBEAT_SNARE "/mnt/remote/myApps/beatbox-wav-files/100059__menegass__gui-drum-snare-soft.wav"

#define ACCBEAT_CYN "/mnt/remote/myApps/beatbox-wav-files/100056__menegass__gui-drum-cyn-hard.wav"
#define ACCBEAT_SNARE "/mnt/remote/myApps/beatbox-wav-files/100060__menegass__gui-drum-splash-hard.wav"
#define ACCBEAT_HIT_HAT "/mnt/remote/myApps/beatbox-wav-files/100062__menegass__gui-drum-tom-hi-hard.wav"

//Manage operation
static int isTerminate;

//Wave file
static wavedata_t stdBeat[MAX_STD_BEAT];
static wavedata_t accBeat[MAX_STD_BEAT];

static int selectedBeat = -1;
static int mode = 0;

static pthread_t audioThreadId;
static pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t modeMutex = PTHREAD_MUTEX_INITIALIZER;

//Initiate private function
void* addThemeToQueue_thread();
static void loadBeatIntoMemory();
static void cleanUpBeatInMemory();

//Music theme
static void playback_stdRockBeat();
static void playback_customBeats();

//Mode
static void play_mode0(void);
static void play_mode1(void);
static void play_mode2(void);


/*
#########################
#        PUBLIC         #
#########################
*/

void AudioMixerControl_init(void)
{
    //Init trigger flag
    isTerminate = 0;

    //Init Audio Mixer
    AudioMixer_init();

    //Load beats into memory
    loadBeatIntoMemory();

    // Launch playback thread:
	if(pthread_create(&audioThreadId, NULL, addThemeToQueue_thread, NULL) != 0)
    {
        exit(EXIT_FAILURE);
    }
}

void AudioMixerControl_join(void)
{
    //Join the thread
    pthread_join(audioThreadId, NULL);
}

void AudioMixerControl_cleanup(void)
{
    //Free other service
    AudioMixer_cleanup();

    //Clean up beat
    cleanUpBeatInMemory();
}

void AudioMixerControl_addDrum(int soundIndex)
{
    AudioMixer_queueSound(&accBeat[soundIndex]);
}

void AudioMixerControl_terminate() 
{
    isTerminate = 1;
}

/////////////////// Sound Control ///////////////////

void AudioMixerControl_setVolume(int newVolume)
{
    AudioMixer_setVolume(newVolume);
}

int AudioMixerControl_getVolume()
{
    return AudioMixer_getVolume();
}

/////////////////// Volume Control ///////////////////

void AudioMixerControl_setTempo(int newTempo)
{
    AudioMixer_setTempo(newTempo);
}

int AudioMixerControl_getTempo()
{
    return AudioMixer_getTempo();
}

/////////////////// User Selection ///////////////////


void AudioMixerControl_controlBeat(int beatIndex)
{
    if(beatIndex < -1 || beatIndex > 2)
    {
        return;
    }

    pthread_mutex_lock(&audioMutex);
    selectedBeat = beatIndex;
    pthread_mutex_unlock(&audioMutex);
}


int AudioMixerControl_hasSound(void)
{
    return AudioMixer_isSoundBites();
}

void AudioMixerControl_setMode(int newMode)
{
    pthread_mutex_lock(&modeMutex);
    mode = newMode;
    pthread_mutex_unlock(&modeMutex);
}


int AudioMixerControl_getMode(void)
{
    pthread_mutex_lock(&modeMutex);
    int tempMode = mode;
    pthread_mutex_unlock(&modeMutex);

    return tempMode;
}


/*
#########################
#       PRIVATE         #
#########################
*/



void* addThemeToQueue_thread()
{
    int selectedMode;
    while(!isTerminate)
    {
        pthread_mutex_lock(&modeMutex);
        selectedMode = mode;
        pthread_mutex_unlock(&modeMutex);

        // Run by mode
        if(selectedMode == 2)
        {
            play_mode2();
            AudioMixerControl_setMode(0);
        }
        else if (selectedMode == 1)
        {
            play_mode1();
            AudioMixerControl_setMode(0);
        }
        else
        {
            play_mode0();
        }
    }

    return NULL;
}

static void play_mode0(void)
{
    int getSelectedBeat;
    pthread_mutex_lock(&audioMutex);
    getSelectedBeat = selectedBeat;
    pthread_mutex_unlock(&audioMutex);

    //Play standard rock beat
    if(getSelectedBeat == 1)
    {
        playback_stdRockBeat();
    }
    //Play custom beat
    else if(getSelectedBeat == 2)
    {
        playback_customBeats();
    }
    //Play None
    else if(getSelectedBeat == 0)
    {
        AudioMixer_CleanUpQueue();
        AudioMixer_CleanUpBuffer();
        AudioMixerControl_controlBeat(-1);
    }
}


static void play_mode1(void)
{
    //Clean -> None
    AudioMixerControl_controlBeat(0);
    sleepForMs(2*convertTempoIntoTime(AudioMixer_getTempo()));

    //Play custom beat
    playback_customBeats();
    sleepForMs(9*convertTempoIntoTime(AudioMixer_getTempo()));

    //Clean -> None
    AudioMixerControl_controlBeat(0);
    sleepForMs(2*convertTempoIntoTime(AudioMixer_getTempo()));
    
    //Play standard rock beat
    playback_stdRockBeat();
    sleepForMs(2*convertTempoIntoTime(AudioMixer_getTempo()));
}

static void play_mode2(void)
{
    //Clean -> None
    AudioMixerControl_controlBeat(0);
    sleepForMs(2*convertTempoIntoTime(AudioMixer_getTempo()));

    playback_stdRockBeat();
    sleepForMs(9*convertTempoIntoTime(AudioMixer_getTempo()));

    playback_customBeats();
    sleepForMs(9*convertTempoIntoTime(AudioMixer_getTempo()));

    //Clean -> None
    AudioMixerControl_controlBeat(0);
    sleepForMs(2*convertTempoIntoTime(AudioMixer_getTempo()));
}

static void loadBeatIntoMemory()
{
    //Load 3 standard beat mode
	AudioMixer_readWaveFileIntoMemory(STDBEAT_DRUM, &stdBeat[0]);
    AudioMixer_readWaveFileIntoMemory(STDBEAT_HIT_HAT, &stdBeat[1]);
    AudioMixer_readWaveFileIntoMemory(STDBEAT_SNARE, &stdBeat[2]);

    //Load 3 beat for accelerometer
    AudioMixer_readWaveFileIntoMemory(ACCBEAT_CYN, &accBeat[0]);                //0 - Z axis - Drum
    AudioMixer_readWaveFileIntoMemory(ACCBEAT_SNARE, &accBeat[1]);              //1 - X axis - Snare
    AudioMixer_readWaveFileIntoMemory(ACCBEAT_HIT_HAT, &accBeat[2]);            //2 - Y axis - Hit-hat 
}

static void cleanUpBeatInMemory()
{
    //Free standard beat
    AudioMixer_freeWaveFileData(&stdBeat[0]);
    AudioMixer_freeWaveFileData(&stdBeat[1]);
    AudioMixer_freeWaveFileData(&stdBeat[2]);

    //Free beat for accelerometer
    AudioMixer_freeWaveFileData(&accBeat[0]);
    AudioMixer_freeWaveFileData(&accBeat[1]);
    AudioMixer_freeWaveFileData(&accBeat[2]);
}


/////////////////////////// MUSIC THEME /////////////////////////// 

//Standard Rock Beat
static void playback_stdRockBeat()
{
    //beat 1 - Hit-hat, base
    AudioMixer_queueSound(&stdBeat[1]);
    AudioMixer_queueSound(&stdBeat[0]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 1 - Hit-hat
    AudioMixer_queueSound(&stdBeat[1]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 2 - Hit-hat, snare
    AudioMixer_queueSound(&stdBeat[1]);
    AudioMixer_queueSound(&stdBeat[2]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 2.5 - Hit-hat
    AudioMixer_queueSound(&stdBeat[1]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 3 - Hit-hat, base
    AudioMixer_queueSound(&stdBeat[1]);
    AudioMixer_queueSound(&stdBeat[0]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 3.5 - Hit-hat
    AudioMixer_queueSound(&stdBeat[1]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 4 - Hit-hat, snare
    AudioMixer_queueSound(&stdBeat[1]);
    AudioMixer_queueSound(&stdBeat[0]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 4.5 - Hit hat
    AudioMixer_queueSound(&stdBeat[1]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));
}

//Standard Rock Beat
static void playback_customBeats()
{
    //beat 1 - base
    AudioMixer_queueSound(&stdBeat[1]);
    AudioMixer_queueSound(&stdBeat[0]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 1.5 - Hit-hat
    AudioMixer_queueSound(&stdBeat[0]);
    AudioMixer_queueSound(&accBeat[2]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 2 - Hit-hat, snare
    AudioMixer_queueSound(&stdBeat[1]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 2.5 - Hit-hat
    AudioMixer_queueSound(&stdBeat[0]);
    AudioMixer_queueSound(&accBeat[1]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 3
    AudioMixer_queueSound(&stdBeat[1]);
    AudioMixer_queueSound(&stdBeat[0]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 3.5
    AudioMixer_queueSound(&stdBeat[0]);
    AudioMixer_queueSound(&accBeat[2]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 4
    AudioMixer_queueSound(&stdBeat[1]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));

    //beat 4.5
    AudioMixer_queueSound(&stdBeat[0]);
    AudioMixer_queueSound(&accBeat[1]);
    sleepForMs(convertTempoIntoTime(AudioMixer_getTempo()));
}