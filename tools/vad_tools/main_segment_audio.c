#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>


//#include "timing.h"

#define DR_WAV_IMPLEMENTATION

#include "dr_wav.h"
// #include "vad.h"

#include "online_vad.h"

#ifndef nullptr
#define nullptr 0
#endif

#ifndef MIN
#define  MIN(A, B)        ((A) < (B) ? (A) : (B))
#endif

#ifndef MAX
#define  MAX(A, B)        ((A) > (B) ? (A) : (B))
#endif


//读取wav文件
int16_t *wavRead_int16(char *filename, uint32_t *sampleRate, uint64_t *totalSampleCount)
{
    unsigned int channels;
    int16_t *buffer = drwav_open_and_read_file_s16(filename, &channels, sampleRate, totalSampleCount);
    if (buffer == nullptr)
    {
        printf("读取wav文件失败.");
    }
    //仅仅处理单通道音频
    if (channels != 1)
    {
        drwav_free(buffer);
        buffer = nullptr;
        *sampleRate = 0;
        *totalSampleCount = 0;
    }
    return buffer;
}


//write wav文件
void wavWrite_int16(char *filename, int16_t *buffer, int sampleRate, uint64_t totalSampleCount, uint32_t channels)
{
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_PCM;
    format.channels = channels;
    format.sampleRate = (drwav_uint32) sampleRate;
    format.bitsPerSample = 16;

    drwav *pWav = drwav_open_file_write(filename, &format);
    if (pWav)
    {
        drwav_uint64 samplesWritten = drwav_write(pWav, totalSampleCount, buffer);
        drwav_uninit(pWav);
        if (samplesWritten != totalSampleCount)
        {
            fprintf(stderr, "write file [%s] error.\n", filename);
            exit(1);
        }
        DRWAV_FREE(pWav);
        pWav = NULL;
    }
}


//分割路径函数
void splitpath(const char *path, char *drv, char *dir, char *name, char *ext)
{
    const char *end;
    const char *p;
    const char *s;
    if (path[0] && path[1] == ':')
    {
        if (drv)
        {
            *drv++ = *path++;
            *drv++ = *path++;
            *drv = '\0';
        }
    }
    else if (drv)
        *drv = '\0';
    for (end = path; *end && *end != ':';)
        end++;
    for (p = end; p > path && *--p != '\\' && *p != '/';)
        if (*p == '.')
        {
            end = p;
            break;
        }
    if (ext)
        for (s = end; (*ext = *s++);)
            ext++;
    for (p = end; p > path;)
        if (*--p == '\\' || *p == '/')
        {
            p++;
            break;
        }
    if (name)
    {
        for (s = p; s < end;)
            *name++ = *s++;
        *name = '\0';
    }
    if (dir)
    {
        for (s = path; s < p;)
            *dir++ = *s++;
        *dir = '\0';
    }
}





void segment_audio(char * conf_file, char *in_file, char *output_dir, int drop_silence)
{


    char drive[3];
    char dir[256];
    char fname[256];
    char ext[256];
    char out_file[1024];
 
    splitpath(in_file, drive, dir, fname, ext);
    // sprintf(out_file, "%s%s%s_out%s", drive, dir, fname, ext);


    //音频采样率
    uint32_t sampleRate = 0;
    //总音频采样数
    uint64_t inSampleCount = 0;

    int16_t *inBuffer = wavRead_int16(in_file, &sampleRate, &inSampleCount);
    //如果加载成功
    if (inBuffer == nullptr){
        printf("read file fault");
        return;
    }
    
    
    Online_Vad_Engine * online_vad_engine = online_vad_engine_create();    
    int init_ret = online_vad_engine_init_from_json(online_vad_engine, conf_file);
    if (init_ret != 0){
        printf("init error \n");
        exit(1);
    }
    int sample_rate;
    int frame_ms;

    int ret1 = online_vad_engine_get_param(online_vad_engine, "sample_rate", (void*) &sample_rate );
    int ret2 = online_vad_engine_get_param(online_vad_engine, "frame_ms", (void*) &frame_ms);
    if (!ret1 || !ret2){
        printf("get param fault\n");
        exit(1);
    }


    // printf("sample_rate: %d\n", sample_rate);
    // printf("frame_ms: %d\n", frame_ms);


    int frame_size = POINT_SIZE * sample_rate / 1000 * frame_ms;
    int frame_count = POINT_SIZE * inSampleCount / frame_size;
    unsigned char * input = (unsigned char *) inBuffer; 
    int bytes_used = frame_count * frame_size;
    unsigned char * required_out = (unsigned char *)malloc(bytes_used);
    int required_out_cur_len = 0;


    int voice_actived_flag = 0;
    int voice_start = 0;
    // printf("start\n");
    // printf("frame likelyhood is:\n");
    // printf("[\n");
  
    for (int i=0; i<frame_count; i++){

        unsigned char * out_buf = NULL;
        int  out_len = 0;
        int ret = online_vad_process(online_vad_engine, input, &out_buf, &out_len);
        input += frame_size;
        // printf("online vad return: %d\n", ret);  
        if (out_buf && out_len){
            memcpy(required_out+required_out_cur_len, out_buf, out_len);
            required_out_cur_len += out_len;
            free(out_buf); 
        }

        if (drop_silence && ret == FLOW_VAD_ACTIVE){
            voice_start = required_out_cur_len-out_len;
            voice_actived_flag = 1;

        }

        if (ret == FLOW_VAD_INACTIVE){           
            uint64_t totalSampleCount = (required_out_cur_len-voice_start) / POINT_SIZE;
            if (totalSampleCount){
                char s_segment_start[100];
                char s_segment_ms[100];
                float segment_start = voice_start/(float)sample_rate*1000 /POINT_SIZE;
                float segment_ms = totalSampleCount/(float)sample_rate*1000;
                sprintf(s_segment_start, "%d", (int)segment_start);
                sprintf(s_segment_ms, "%d", (int)segment_ms);
                // printf("voice_start: %s\n", s_segment_start);
                // printf("s_segment_ms: %s\n", s_segment_ms);
                sprintf(out_file, "%s/%s_out_%s_%s%s", output_dir, fname, s_segment_start, s_segment_ms, ext);
                // printf("out file path: %s\n",out_file);
                int16_t * buffer = (int16_t *)(required_out+voice_start);
                wavWrite_int16(out_file, buffer, sample_rate, totalSampleCount, 1);
                voice_start = required_out_cur_len;
                voice_actived_flag = 0;
            }
        }

    }


    unsigned char * out_buf = NULL;
    int  out_len = 0;
    online_vad_flush(online_vad_engine,  &out_buf, &out_len);
    if (out_buf && out_len){
        memcpy(required_out+required_out_cur_len, out_buf, out_len);
        required_out_cur_len += out_len;
        free(out_buf);
    }

    //  write last segment
    uint64_t totalSampleCount = (required_out_cur_len-voice_start) / POINT_SIZE;

    if (totalSampleCount && ((drop_silence && voice_actived_flag) || !drop_silence)){
        
        char s_segment_start[100];
        char s_segment_ms[100];
        float segment_start = voice_start/(float)sample_rate*1000 /POINT_SIZE;
        float segment_ms = totalSampleCount/(float)sample_rate*1000;
        sprintf(s_segment_start, "%d", (int)segment_start);
        sprintf(s_segment_ms, "%d", (int)segment_ms);

        sprintf(out_file, "%s/%s_out_%s_%s%s", output_dir, fname, s_segment_start, s_segment_ms, ext);
               
        int16_t * buffer = (int16_t *)(required_out+voice_start);
        wavWrite_int16(out_file, buffer, sample_rate, totalSampleCount, 1);
        voice_start = required_out_cur_len;
    }

    online_vad_engine_destory(online_vad_engine);

    free(inBuffer);
    free(required_out);   
}



void showUsage(){
    printf("\nthis program will segment the wav file by vad algorithm, and put outputs to directory same as input wav file\n");
    printf("usage example: \n");
    printf("./audio_segment  --conf-file conf.json  in.wav output_dir \n");
    printf("option --drop-silence can be added to drop the silence voice in wav \n\n");
    printf("other option should be set in json file like conf/conf.json, the option include:\n");
    printf("vad_mode :  the likelyhood of voice-active frame, "
            "\n      value range: 0, 1, 2, 3.   3 is very likely."
            "\nsample_rate : sample rate of input audio."
            "\nframe_ms : the time lenght of a frame, "
            "\n       value range: 10, 20, 30; corresponding rate:8000, 16000, 32000"
            "\nactive_queue_size : the number of frames in the queue to diagnose voice start event."
            "\nactive_threshold : the threshold of proportion of active frame in the queue to "
            "\n       triger voice start event. value range: [0, 1]"
            "\ninactive_queue_size : the number of frames of the queue to diagnose voice end event."
            "\ninactive_threshold : the threshold of proportion of inacitve frame in the queue to "
            "\n       triger voice end event. value range: [0, 1]\n");
}


int CreateDirectoryEx( char *sPathName )  
{  
    char DirName[256];      
    strcpy(DirName,sPathName);      
    int i,len = strlen(DirName);      
    if(DirName[len-1]!='/')      
    strcat(DirName,"/");            
    len = strlen(DirName);      
    for(i=1;i<len;i++)      
    {      
        if(DirName[i]=='/')      
        {      
            DirName[i] = 0;      
            if(access(DirName,NULL) != 0)      
            {      
                if(mkdir(DirName,0777) == -1)      
                {       
                    perror("mkdir error");       
                    return -1;       
                }      
            }    
            DirName[i] = '/';      
         }      
  }        
  return 0;      
}


int main(int argc, char *argv[])
{
    if (argc<4){
        showUsage();
        return 1;
    }

    char * infile = argv[argc-2];
    char * output_dir = argv[argc-1];
    if (output_dir[strlen(output_dir)-1] == '/'){
        output_dir[strlen(output_dir)-1]='\0';
    }
    CreateDirectoryEx(output_dir);

    argc = argc-2;

    // printf("wav segment tool using webrtc vad algarithm\n");
    static struct option longOpts[] = {
        // { "file", required_argument, NULL, 'f' },
        { "conf-file", required_argument, NULL, 'c' },
        { "drop-silence", no_argument, NULL, 'd' },
        { "help", no_argument, NULL, 'h' },
        { 0, 0, 0, 0 }
    };

    char * conf_file = NULL;
    // char *in_file = NULL;
    int drop_silence = 0;

    int opt;
    // int option_index = 0;

    while ( (opt = getopt_long(argc, argv, "c:dh", longOpts, NULL)) != -1)
    {

        switch(opt) {
            case 'c':
            conf_file = optarg;
            break;
            case 'd':
            drop_silence = 1;
            break;
            case 'h':
            showUsage();
            exit(0);
            default:
            showUsage();
            exit(1);        
        }

    }
    if (!conf_file){
        printf("error:conf_file parameter missing. exit\n");
        showUsage();
        exit(1);
    }

    // printf("\n\n#########################\n");
    // printf("parameter get from command line: \n");
	// printf("#########################\n");
    // printf("conf_file: %s\n",conf_file);
    // printf("drop_silence: %d\n",drop_silence);
	// printf("#########################\n\n");

    segment_audio(conf_file, infile, output_dir, drop_silence);
    return 0;

}
