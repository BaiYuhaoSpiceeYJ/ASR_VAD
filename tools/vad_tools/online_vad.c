#include "online_vad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#define LOG4C_LEVEL_ERROR
#include "log4c.h"

struct _Online_Vad_Engine{
    void * _vad_engine;    //weibrtc vad engine
    int _active_flag;
    int  _active_queue_size;  //active
    float  _active_threshold;   //acvtive [0~1]
    int  _inactive_queue_size;  //inactive
    float _inactive_threshold;   //inactive [0~1]
    Vad_Active_Queue * _active_queue;
    Vad_InActive_Queue * _inactive_queue;
    uint32_t _sample_rate;
    int _frame_points;   //sample points of a frame
    int _frame_ms;   //time length of a frame
    int _frame_size;  //bytes of a frame
    int _keep_weight;
	int _cur_voice_time_length;
	int _voice_timeout_shreshold;
	int _cur_silent_time_length;
	int _silent_timeout_shreshold;
};

int online_vad_engine_get_param(Online_Vad_Engine * online_vad_engine, char * param_name, void * param){
	if (strcmp(param_name,"sample_rate") == 0){
		int *_param = (int *) param;
		*_param = online_vad_engine->_sample_rate;
	}else if(strcmp(param_name,"frame_ms") == 0){
		int *_param = (int *) param;
		*_param = online_vad_engine->_frame_ms;
	}else{
		LOG_ERROR("unknown param name\n");
		return OK;
	}
	return ERROR;

}

Online_Vad_Engine * online_vad_engine_create()
{
    Online_Vad_Engine * online_vad_engine = (Online_Vad_Engine * )malloc(sizeof(Online_Vad_Engine));
    online_vad_engine->_vad_engine = WebRtcVad_Create();
    if (online_vad_engine->_vad_engine == NULL) {
		LOG_ERROR("WebRtcVad_Create() fail\n");
        return NULL;
    }

    return online_vad_engine;
}




int online_vad_engine_init(Online_Vad_Engine * online_vad_engine,
                                    int webrtc_vad_mode,
                                    int webrtc_vad_sampleRate,
                                    int webrtc_vad_frame_ms,
                                    int active_queue_size, 
                                    float active_threshold, 
                                    int  inactive_queue_size, 
                                    float inactive_threshold)
{
	int status = WebRtcVad_Init(online_vad_engine->_vad_engine);
    if (status != 0)
    {
		LOG_ERROR("WebRtcVad_Init() fail\n");
        WebRtcVad_Free(online_vad_engine->_vad_engine);
        return ERROR;
    }
	status = WebRtcVad_set_mode(online_vad_engine->_vad_engine, webrtc_vad_mode);
    if (status != 0)
    {
		LOG_ERROR("WebRtcVad_set_mode() fail\n");
        WebRtcVad_Free(online_vad_engine->_vad_engine);
        return ERROR;
    }

	online_vad_engine->_sample_rate = webrtc_vad_sampleRate;
    online_vad_engine->_active_queue_size = active_queue_size;
    online_vad_engine->_active_threshold = active_threshold;
    online_vad_engine->_inactive_queue_size = inactive_queue_size;
    online_vad_engine->_inactive_threshold = inactive_threshold;
    online_vad_engine->_keep_weight = 0;
    online_vad_engine->_active_flag = 0;
    online_vad_engine->_frame_ms = webrtc_vad_frame_ms;
    online_vad_engine->_frame_points = webrtc_vad_sampleRate / 1000 * webrtc_vad_frame_ms;
    online_vad_engine->_frame_size = POINT_SIZE * webrtc_vad_sampleRate / 1000 * webrtc_vad_frame_ms;
	online_vad_engine->_cur_voice_time_length = 0;
	online_vad_engine->_voice_timeout_shreshold = 0;
	online_vad_engine->_cur_silent_time_length = 0;
	online_vad_engine->_silent_timeout_shreshold = 0;

    online_vad_engine->_active_queue = CreatVadActiveQueue(online_vad_engine, active_queue_size);
    if (!online_vad_engine->_active_queue){
		LOG_ERROR("CreatVadActiveQueue() failed.\n");
        return ERROR;
    }
    online_vad_engine->_inactive_queue = CreatVadInActiveQueue(online_vad_engine, inactive_queue_size);
    if (!online_vad_engine->_inactive_queue){
		LOG_ERROR("CreatVadInActiveQueue() failed\n");
        return ERROR;
    }
	// LOG_INFO("online_vad_engine inited, the params setting are as below: \n");
    // online_vad_engine_info_print(online_vad_engine);
	return OK;
}



char *json_loader(char *path)
{
  FILE *f;
  long len;
  char *content;
  f=fopen(path,"rb");
  fseek(f,0,SEEK_END);
  len=ftell(f);
  fseek(f,0,SEEK_SET);
  content=(char*)malloc(len+1);
  fread(content,1,len,f);
  fclose(f);
  return content;
}
int online_vad_engine_init_from_json(Online_Vad_Engine * online_vad_engine, char * file_name){
	int webrtc_vad_mode = 3;
	int webrtc_vad_sampleRate = 8000;
	int webrtc_vad_frame_ms = 20;
	int active_queue_size = 10;
	float active_threshold = 0.8;
	int inactive_queue_size = 25;
	float inactive_threshold = 0.85;

	char *json_str = json_loader(file_name);
	cJSON * json = cJSON_Parse(json_str);
	cJSON * _webrtc_vad_mode = cJSON_GetObjectItem(json, "vad_mode");
    cJSON * _webrtc_vad_sampleRate = cJSON_GetObjectItem(json, "sample_rate");
	cJSON * _webrtc_vad_frame_ms = cJSON_GetObjectItem(json, "frame_ms");
	cJSON * _active_queue_size = cJSON_GetObjectItem(json, "active_queue_size");
	cJSON * _active_threshold = cJSON_GetObjectItem(json, "active_threshold");
	cJSON * _inactive_queue_size = cJSON_GetObjectItem(json, "inactive_queue_size");
	cJSON * _inactive_threshold = cJSON_GetObjectItem(json, "inactive_threshold");

	if (!json || json->type != cJSON_Object){
		LOG_ERROR("json parse error\n");
	}

	if (_webrtc_vad_mode && _webrtc_vad_mode->type == cJSON_Number){
		webrtc_vad_mode = _webrtc_vad_mode->valueint;
	}else{
		LOG_ERROR("parse param webrtc_vad_mode error\n");
	}

	if (_webrtc_vad_sampleRate && _webrtc_vad_sampleRate->type == cJSON_Number){
		webrtc_vad_sampleRate = _webrtc_vad_sampleRate->valueint;
	}else{
		LOG_ERROR("parse param webrtc_vad_sampleRate error\n");
	}
	
	if (_webrtc_vad_frame_ms && _webrtc_vad_frame_ms->type == cJSON_Number){
		webrtc_vad_frame_ms = _webrtc_vad_frame_ms->valueint;
	}else{
		LOG_ERROR("parse param webrtc_vad_frame_ms error\n");
	}
	
	if ( _active_queue_size &&  _active_queue_size->type == cJSON_Number){
		 active_queue_size =  _active_queue_size->valueint;
	}else{
		LOG_ERROR("parse param  active_queue_size error\n");
	}	

	if (_active_threshold && _active_threshold->type == cJSON_Number){
		active_threshold = (float)(_active_threshold->valuedouble);
	}else{
		LOG_ERROR("parse param active_threshold error\n");
	}	

	if (_inactive_queue_size && _inactive_queue_size->type == cJSON_Number){
		inactive_queue_size = _inactive_queue_size->valueint;
	}else{
		LOG_ERROR("parse param inactive_queue_size error\n");
	}	

	if (_inactive_threshold && _inactive_threshold->type == cJSON_Number){
		inactive_threshold = (float)(_inactive_threshold->valuedouble);
	}else{
		LOG_ERROR("parse param inactive_threshold error\n");
	}				
	cJSON_Delete(json);
	free(json_str);

	return online_vad_engine_init(online_vad_engine,
                            		webrtc_vad_mode,
                                    webrtc_vad_sampleRate,
                                    webrtc_vad_frame_ms,
                                    active_queue_size, 
                                    active_threshold, 
                                    inactive_queue_size, 
                                    inactive_threshold);
}




int online_vad_engine_enable_silent_timeout(Online_Vad_Engine * online_vad_engine, int silent_timeout_shreshold){
	if (online_vad_engine){
		online_vad_engine->_silent_timeout_shreshold = silent_timeout_shreshold;
		return 1;
	}
	return 0;
}

int online_vad_engine_enable_voice_timeout(Online_Vad_Engine * online_vad_engine, int voice_timeout_shreshold){
	if (online_vad_engine){
		online_vad_engine->_voice_timeout_shreshold = voice_timeout_shreshold;
		return 1;
	}
	return 0;
}



int online_vad_process(Online_Vad_Engine* engine, 
                                const unsigned char * in_buf, 
                                unsigned char ** out_buf, 
                                int * out_len)
{
    int ret = FLOW_OK;
	int _ret;

    if (!engine->_active_flag){
        _ret = Vad_Process_Active_EnQueue(engine->_active_queue,  in_buf, out_buf, out_len);
		engine->_cur_silent_time_length += engine->_frame_ms;
		if (engine->_silent_timeout_shreshold && engine->_cur_silent_time_length >= engine->_silent_timeout_shreshold){
			// printf("--[silent_timeout]--");
			ret = FLOW_SLIENT_TIMEOUT;
			engine->_cur_silent_time_length = 0;
		}
		
        if (_ret == FLOW_VAD_ACTIVE){
			// printf("--[active]--");
			ret = FLOW_VAD_ACTIVE;
            engine->_active_flag = 1;
			if (engine->_silent_timeout_shreshold)
				engine->_cur_silent_time_length = 0;
        }
		
    }else{
        _ret = Vad_Process_InActive_EnQueue(engine->_inactive_queue,  in_buf, out_buf, out_len); 
		   engine->_cur_voice_time_length += engine->_frame_ms;
		if (engine->_voice_timeout_shreshold && engine->_cur_voice_time_length >= engine->_voice_timeout_shreshold){
			// printf("--[voice_timeout]--");
			ret = FLOW_VOICE_TIMEOUT;
			engine->_cur_voice_time_length = 0;

		}
        if (_ret == FLOW_VAD_INACTIVE){
			// printf("--[inactive]--");
			ret = FLOW_VAD_INACTIVE;
            engine->_active_flag = 0;
			if (engine->_voice_timeout_shreshold)
				engine->_cur_voice_time_length = 0;
        }
    }
    return ret;
}



void online_vad_flush(Online_Vad_Engine* engine,  
                                unsigned char ** out_buf, 
                                int * out_len)
{	
	
    ActiveQueueClear(engine->_active_queue, out_buf, out_len);
	
}




int online_vad_engine_destory(Online_Vad_Engine* engine)
{
    ActiveQueueDestroy(engine->_active_queue);
    InActiveQueueDestroy(engine->_inactive_queue);
	WebRtcVad_Free(engine->_vad_engine);
    free(engine);
	return OK;
}



void online_vad_engine_info_print(Online_Vad_Engine* online_vad_engine){
    printf("online_vad_engine->_sample_rate: %d\n", online_vad_engine->_sample_rate);
    printf("online_vad_engine->_active_queue_size: %d\n", online_vad_engine->_active_queue_size);
    printf("online_vad_engine->_active_threshold: %f\n", online_vad_engine->_active_threshold);
    printf("online_vad_engine->_inactive_queue_size: %d\n", online_vad_engine->_inactive_queue_size);
    printf("online_vad_engine->_inactive_threshold: %f\n", online_vad_engine->_inactive_threshold);
    printf("online_vad_engine->_keep_weight: %d\n", online_vad_engine->_keep_weight);
    printf("online_vad_engine->_active_flag: %d\n", online_vad_engine->_active_flag);
    printf("online_vad_engine->_frame_ms: %d\n", online_vad_engine->_frame_ms);
    printf("online_vad_engine->_frame_points: %d\n", online_vad_engine->_frame_points);
    printf("online_vad_engine->_frame_size: %d\n", online_vad_engine->_frame_size);
}







struct _Vad_Active_Queue{
	Online_Vad_Engine * _online_vad_engine;
    unsigned char * _buffer_array;
	int * _likelyhood_array;
    int _queue_size;
	int _front;
	int _rear;
	float _cur_active_levels; //active likehood level in queue, [0~1]
};


Vad_Active_Queue * CreatVadActiveQueue(Online_Vad_Engine * online_vad_engine, int queue_size){
    Vad_Active_Queue * q = (Vad_Active_Queue * )malloc(sizeof(Vad_Active_Queue));
    q->_queue_size = queue_size+1;  //use extra one buffer_item to judge queue full;
    q->_front = 0;
	q->_rear = 0;
	q->_online_vad_engine = online_vad_engine;
    q->_buffer_array = (unsigned char *) malloc(q->_queue_size * online_vad_engine->_frame_size);
	q->_likelyhood_array = (int *) malloc(q->_queue_size * sizeof(int));
	q->_cur_active_levels = 0;
    if (q->_buffer_array==NULL || q->_likelyhood_array==NULL)
		exit(OVER_FLOW);
	return q;
}

/*return the length of the queue*/
int ActiveQueueLength(Vad_Active_Queue *q) {
	return (q->_rear - q->_front + q->_queue_size) % q->_queue_size;
}
/*Destroy the queue*/
void ActiveQueueDestroy(Vad_Active_Queue *q) {
    free(q->_buffer_array);
	q->_buffer_array = NULL;
	free(q->_likelyhood_array);
	q->_likelyhood_array = NULL;
	free(q);
    q = NULL;
}

/*determine if the queue is empty*/
int ActiveQueueIsEmpty(Vad_Active_Queue *q) {
	return q->_rear == q->_front;
}

int ActiveQueueIsFull(Vad_Active_Queue *q) {
	return (q->_rear + 1) % q->_queue_size == q->_front;
}


/*clean the queue, pop all the items*/
void ActiveQueueClear(Vad_Active_Queue *q, unsigned char ** out_buffer, int *out_len){
	if (out_buffer && out_len){
		*out_len = ActiveQueueLength(q) * q->_online_vad_engine->_frame_size;
		if (*out_len){
			*out_buffer = (unsigned char *)malloc(*out_len);
			if (q->_rear >= q->_front){
				memcpy(*out_buffer, q->_buffer_array+q->_front*q->_online_vad_engine->_frame_size, *out_len);                
			}else{
				//q->_front start form 0
				memcpy(*out_buffer, q->_buffer_array+q->_front*q->_online_vad_engine->_frame_size, (q->_queue_size-q->_front)*q->_online_vad_engine->_frame_size); 
				if (q->_rear) //the index q->_rear start form 0, and q->rear does'nt point the actual buffer in queue
					memcpy(*out_buffer+(q->_queue_size-q->_front)*q->_online_vad_engine->_frame_size, q->_buffer_array, q->_rear*q->_online_vad_engine->_frame_size); 
			}
		}else{
			*out_buffer = NULL;
		}
	}
	q->_front = q->_rear = 0;
	q->_cur_active_levels = 0;
}


//in_buffer_item must be the size of q->_buffer_item_size
int Vad_Process_Active_EnQueue(Vad_Active_Queue *q, const unsigned char * in_buffer_item, unsigned char ** out_buffer, int *out_len)
{	
	int nVadRet = WebRtcVad_Process(q->_online_vad_engine->_vad_engine, 
	                            	q->_online_vad_engine->_sample_rate, 
									(const short int *)in_buffer_item, 
					        		q->_online_vad_engine->_frame_points, 
									q->_online_vad_engine->_keep_weight);
	// printf("--%d--", nVadRet);


	
	if (ActiveQueueIsFull(q)){
		int out_likely_hood;
		ActiveDeQueue(q, out_buffer, out_len, &out_likely_hood);
		if (out_likely_hood>0)
			q->_cur_active_levels -= 1.0 / (q->_queue_size-1);
	}else{    
		*out_buffer = NULL;  
		*out_len = 0;
	}

	ActiveEnQueue(q, in_buffer_item, nVadRet);
	int vad_actived = 0;
	if (nVadRet > 0){
		q->_cur_active_levels += 1.0 / (q->_queue_size-1);
		if(q->_cur_active_levels >= q->_online_vad_engine->_active_threshold){
			vad_actived = 1;   //current frame is actived
		}
	}

	if (!vad_actived){
		return FLOW_OK;
	}	
	/*
	*out_buffer may point to a memeroy already allocated 
	by fun ActiveDeQueue(), if not, *out_buffer=NULL and *out_len=0
	*/
	int q_len_bytes = ActiveQueueLength(q)*q->_online_vad_engine->_frame_size;
	int tmp_len = *out_len+q_len_bytes;
	unsigned char * tmp_buffer = (unsigned char *)malloc(tmp_len);
	unsigned char * _p = (unsigned char *)(tmp_buffer+*out_len);
	if (*out_buffer && *out_len){
		memcpy(tmp_buffer, *out_buffer, *out_len);
		free(*out_buffer);
	}
	int cpy_ret = ActiveQueueCopyData(q, &_p, q_len_bytes, 0);
	ActiveQueueClear(q, NULL, NULL);

	if (cpy_ret == ERROR){
		LOG_ERROR("data copy error\n");
		*out_buffer = NULL;  
		*out_len = 0;	
		return FLOW_ERROR;
	}
	*out_buffer = tmp_buffer;
	* out_len = tmp_len;

    //vad actived, flush the buffer inside the queue out
	return FLOW_VAD_ACTIVE;
}


/*enqueue, insert the rear*/
int ActiveEnQueue(Vad_Active_Queue *q, const unsigned char *buffer, int vad_likely_hood){  // item e must be size of buffer_item_size
	if (ActiveQueueIsFull(q))
		return FULL_ENQUEUE;
    memcpy(q->_buffer_array+q->_rear * q->_online_vad_engine->_frame_size , buffer, q->_online_vad_engine->_frame_size);
	q->_likelyhood_array[q->_rear] = vad_likely_hood;
	q->_rear = (q->_rear + 1) % q->_queue_size;	
	return OK;
}


/*dequeue, pop the front*/
int ActiveDeQueue(Vad_Active_Queue *q, unsigned char ** out_buffer, int * out_len, int * likelyhood) {
	if(ActiveQueueIsEmpty(q))
		return EMPTYP_DEQUEUE;
	if (out_buffer && out_len){
		*out_buffer = (unsigned char *)malloc(q->_online_vad_engine->_frame_size);
		memcpy(*out_buffer, q->_buffer_array+q->_front*q->_online_vad_engine->_frame_size, q->_online_vad_engine->_frame_size);
		*out_len = q->_online_vad_engine->_frame_size;
		if (likelyhood)
			*likelyhood = q->_likelyhood_array[q->_front];
	}
    q->_front = (q->_front + 1) % q->_queue_size;	 
	return OK;
}


int ActiveQueueCopyData(Vad_Active_Queue *q, unsigned char ** out_buffer, int required_len, int malloc_flag){
	if (out_buffer && required_len>0){
		int q_len = ActiveQueueLength(q) * q->_online_vad_engine->_frame_size;
		if (malloc_flag)
			*out_buffer = (unsigned char *)malloc(required_len);

		if (required_len > q_len){
			LOG_ERROR("required byte length too large\n");
			* out_buffer = NULL;
			return ERROR;
		}
		int pre_seg_len = (q->_queue_size-q->_front)*q->_online_vad_engine->_frame_size;		
		if (pre_seg_len >= required_len){
			memcpy(*out_buffer, q->_buffer_array+q->_front*q->_online_vad_engine->_frame_size, required_len);                
		}else{
			//q->_front start form 0
			memcpy(*out_buffer, q->_buffer_array+q->_front*q->_online_vad_engine->_frame_size, pre_seg_len); 
			//the index q->_rear start form 0, and q->rear does'nt point the actual buffer in queue
			memcpy(*out_buffer+pre_seg_len, q->_buffer_array, required_len-pre_seg_len); 
		}
		
	}
	return OK;
}






struct _Vad_InActive_Queue{
	Online_Vad_Engine * _online_vad_engine;
	int _queue_size;
	int * _likelyhood_array;
	int _front;
	int _rear;
    float _cur_inactive_levels; //inactive likehood level in queue, [0~1]
};

Vad_InActive_Queue * CreatVadInActiveQueue(Online_Vad_Engine * online_vad_engine, int queue_size) {
	Vad_InActive_Queue * q = (Vad_InActive_Queue * )malloc(sizeof(Vad_InActive_Queue));
    q->_queue_size = queue_size+1;  //use extra one buffer_item to judge queue full;
    q->_front = 0;
	q->_rear = 0;
	q->_likelyhood_array = (int *) malloc(q->_queue_size * sizeof(int));
	q->_cur_inactive_levels = 0;
	q->_online_vad_engine = online_vad_engine;
	return q;
}

/*return the length of the queue*/
int InActiveQueueLength(Vad_InActive_Queue *q) {
	return (q->_rear - q->_front + q->_queue_size) % q->_queue_size;
}
/*Destroy the queue*/
void InActiveQueueDestroy(Vad_InActive_Queue *q){
	free(q->_likelyhood_array);
	q->_likelyhood_array = NULL;
	q->_rear = 0;
	q->_front = 0;
	free(q);
	q = NULL;
}

/*determine if the queue is empty*/
int InActiveQueueIsEmpty(Vad_InActive_Queue *q) {
	return q->_rear == q->_front;
	
}

int InActiveQueueIsFull(Vad_InActive_Queue *q) {
	return (q->_rear + 1) % q->_queue_size == q->_front;
}


void InActiveQueueClear(Vad_InActive_Queue *q){
	q->_front = q->_rear = 0;
	q->_cur_inactive_levels = 0;
}

/*Vad process the input buffer, input buffer must be size of buffer_item_size*/
int Vad_Process_InActive_EnQueue(Vad_InActive_Queue *q, const unsigned char * in_buffer_item, unsigned char ** out_buffer, int *out_len)
{	
	*out_buffer = (unsigned char *)malloc(q->_online_vad_engine->_frame_size);
	memcpy(*out_buffer, in_buffer_item, q->_online_vad_engine->_frame_size);
	*out_len = q->_online_vad_engine->_frame_size;
	int nVadRet = WebRtcVad_Process(q->_online_vad_engine->_vad_engine, 
	                            	q->_online_vad_engine->_sample_rate, 
									(const short int *)in_buffer_item, 
					        		q->_online_vad_engine->_frame_points, 
									q->_online_vad_engine->_keep_weight);
    // printf("--%d--", nVadRet);
	if (InActiveQueueIsFull(q)){
		int out_likely_hood=1;
		InActiveDeQueue(q, &out_likely_hood);
		if (out_likely_hood == 0)
			q->_cur_inactive_levels -= 1.0 / (q->_queue_size-1);
	}

	InActiveEnQueue(q, nVadRet);
	int vad_inactived = 0;
	if (nVadRet == 0){
		q->_cur_inactive_levels += 1.0 / (q->_queue_size-1);
		if(q->_cur_inactive_levels >= q->_online_vad_engine->_inactive_threshold){
			vad_inactived = 1;   //current frame is actived
		}
	}
	
	if (vad_inactived){
		InActiveQueueClear(q);
		return FLOW_VAD_INACTIVE;
	}
	return FLOW_OK;
}



/*enqueue, insert the rear*/
int InActiveEnQueue(Vad_InActive_Queue *q, int item) {
	if (InActiveQueueIsFull(q))
		return FULL_ENQUEUE;
	q->_likelyhood_array[q->_rear] = item;
	q->_rear = (q->_rear + 1) % q->_queue_size;
	return OK;
}


/*dequeue, pop the front*/
int InActiveDeQueue(Vad_InActive_Queue *q, int *item) {
	if(InActiveQueueIsEmpty(q))
		return EMPTYP_DEQUEUE;
	*item = q->_likelyhood_array[q->_front];
	q->_front = (q->_front + 1) % q->_queue_size;
	return OK;
}

