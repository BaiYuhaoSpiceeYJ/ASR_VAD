#ifndef ONLINE_VAD_H_
#define ONLINE_VAD_H_

#include "vad.h"
#include <stdint.h>

#define POINT_SIZE 2 // bytes of each sample point

typedef struct _Vad_Active_Queue Vad_Active_Queue;
typedef struct _Vad_InActive_Queue Vad_InActive_Queue;
typedef struct _Online_Vad_Engine Online_Vad_Engine;



//vad flag
#define FLOW_OK 0
#define FLOW_OK_FLUSH 1
#define FLOW_VAD_ACTIVE 2
#define FLOW_VAD_INACTIVE 3
#define FLOW_ERROR 4
#define FLOW_VOICE_TIMEOUT 5
#define FLOW_SLIENT_TIMEOUT 6


//queue flag
#define EMPTYP_DEQUEUE 7
#define FULL_ENQUEUE 8
#define OVER_FLOW 9


#define OK 0
#define ERROR 1





/*
create the online vad engine, and allocate the resources.
*/
Online_Vad_Engine * online_vad_engine_create();



/*
get param by name
*/
int online_vad_engine_get_param(Online_Vad_Engine * online_vad_engine, char * param_name, void * param);




/*
params
{
webrtc_vad_mode :  the likelyhood of voice-active frame, 
        value range: 0, 1, 2, 3.   3 is very likely.
webrtc_vad_sampleRate : sample rate of input audio
webrtc_vad_frame_ms : the time lenght of a frame, value range: 10, 20, 30.
active_queue_size : the number of frames of the queue to diagnose voice start event.
active_threshold : the threshold of proportion of active frame in the queue to 
        triger voice start event. value range: [0, 1]
inactive_queue_size : the number of frames of the queue to diagnose voice end event.
inactive_threshold : the threshold of proportion of inacitve frame in the queue to 
        triger voice end event. value range: [0, 1]
}
*/
int online_vad_engine_init(Online_Vad_Engine * online_vad_engine,
                                    int webrtc_vad_mode,
                                    int webrtc_vad_sampleRate,
                                    int webrtc_vad_frame_ms,
                                    int active_queue_size, 
                                    float active_threshold, 
                                    int  inactive_queue_size, 
                                    float inactive_threshold);








/*
init online vad engine by parsing json file.
*/
int online_vad_engine_init_from_json(Online_Vad_Engine * online_vad_engine, char * file_name);




/*
this function enable online vad engine to return FLOW_SLIENT_TIMEOUT event.
params
{
        silent_timeout_shreshold: the time length(ms) to tigger FLOW_SLIENT_TIMEOUT event.
}
note: if the frame both trigger FLOW_SLIENT_TIMEOUT event and FLOW_VAD_ACTIVE event, this fuction 
        return FLOW_VAD_ACTIVE event.
*/
int online_vad_engine_enable_silent_timeout(Online_Vad_Engine * online_vad_engine, int silent_timeout_shreshold);





/*
this function enable online vad engine to return FLOW_VOICE_TIMEOUT event.
params
{
        voice_timeout_shreshold: the time length(ms) to tigger FLOW_VOICE_TIMEOUT event.
}
note: if the frame both trigger FLOW_VOICE_TIMEOUT event and FLOW_VAD_INACTIVE event, this fuction 
        return FLOW_VAD_INACTIVE event.
*/
int online_vad_engine_enable_voice_timeout(Online_Vad_Engine * online_vad_engine, int voice_timeout_shreshold);





/*
params
{
in_buf : the input frames with time length of webrtc_vad_frame_ms.
out_buf & out_len : 'out_buf' point to the output of frame data with 
        'out_len' byte length  after this proccess, give NULL if customer 
        do not want data back;
}
*/
int online_vad_process(Online_Vad_Engine* engine, 
                                const unsigned char * in_buf, 
                                unsigned char ** out_buf, 
                                int * out_len);





/*
flush out the data cached in vad process;
params
{
out_buf & out_len : 'out_buf' given to point to the output of frame data with 
        'out_len' byte length  cached in vad process.
}
*/
void online_vad_flush(Online_Vad_Engine* engine,  
                                unsigned char ** out_buf, 
                                int * out_len);



/*
free the resource of online vad proccess.
*/
int online_vad_engine_destory(Online_Vad_Engine* engine);





/*
print the setting of online vad process.
*/
void online_vad_engine_info_print(Online_Vad_Engine* engine);




/*initialize the queue*/
Vad_Active_Queue * CreatVadActiveQueue(Online_Vad_Engine * online_vad_engine, int queue_size);

/*return the length of the queue*/
int ActiveQueueLength(Vad_Active_Queue *q);

/*Destroy the queue*/
void ActiveQueueDestroy(Vad_Active_Queue *q);

/*determine if the queue is empty*/
int ActiveQueueIsEmpty(Vad_Active_Queue *q);

/*determine if the queue is full*/
int ActiveQueueIsFull(Vad_Active_Queue *q);

//*clear the queue;
void ActiveQueueClear(Vad_Active_Queue *q, unsigned char ** out_buffer, int *out_len);


// /*return the head elem of the queue*/
// unsigned char Top(Vad_Start_Queue q);

// /*return the back elem of the queue*/
// Item Back(Vad_Start_Queue q);

/*enqueue, Vad process the input buffer, input buffer must be size of buffer_item_size*/
int Vad_Process_Active_EnQueue(Vad_Active_Queue *q, const unsigned char * in_buffer_item, unsigned char ** out_buffer, int *out_len);


/*enqueue, insert the rear, input buffer must be size of buffer_item_size*/
int ActiveEnQueue(Vad_Active_Queue *q, const unsigned char *buffer, int vad_likely_hood) ;

/*dequeue, pop the front*/
int ActiveDeQueue(Vad_Active_Queue *q, unsigned char ** out_buffer, int * out_len, int * likelyhood);

/*copy continuous data out with given byte-length */
int ActiveQueueCopyData(Vad_Active_Queue *q, unsigned char ** out_buffer, int required_len, int malloc_flag);

// /*print the queue*/
// void PrintQueue(Vad_Active_Queue q);








/*initialize the queue*/
Vad_InActive_Queue * CreatVadInActiveQueue(Online_Vad_Engine * online_vad_engine, int queue_size);

/*return the length of the queue*/
int InActiveQueueLength(Vad_InActive_Queue *q);

/*Destroy the queue*/
void InActiveQueueDestroy(Vad_InActive_Queue *q);

/*determine if the queue is empty*/
int InActiveQueueIsEmpty( Vad_InActive_Queue *q);

/*determine if the queue is full*/
int InActiveQueueIsFull(Vad_InActive_Queue *q);

// /*return the head elem of the queue*/
// Item Top(Queue *q);

// /*return the back elem of the queue*/
// Item Back(Queue *q);

void InActiveQueueClear(Vad_InActive_Queue *q);

/*Vad process the input buffer, input buffer must be size of buffer_item_size*/
int Vad_Process_InActive_EnQueue(Vad_InActive_Queue *q, const unsigned char * in_buffer_item, unsigned char ** out_buffer, int *out_len);

/*enqueue, insert the rear*/
int InActiveEnQueue(Vad_InActive_Queue *q, int item);

/*dequeue, pop the front*/
int InActiveDeQueue(Vad_InActive_Queue *q, int *item);




#endif