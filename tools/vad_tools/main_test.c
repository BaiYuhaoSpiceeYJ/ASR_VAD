#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include "timing.h"
//采用https://github.com/mackron/dr_libs/blob/master/dr_wav.h 解码
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



void online_vad_test(char *in_file)
{

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
    int init_ret = online_vad_engine_init_from_json(online_vad_engine, "../conf/conf.json");
    if (init_ret != 0){
        printf("init error \n");
        exit(1);
    }

    // online_vad_engine_enable_silent_timeout(online_vad_engine, 40);
    // online_vad_engine_enable_voice_timeout(online_vad_engine, 40);

    
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

    for (int i=0; i<frame_count; i++){
        
        unsigned char * out_buf = NULL;
        int  out_len = 0;
        int ret = online_vad_process(online_vad_engine, 
                                input, 
                                &out_buf, 
                                &out_len);
        input += frame_size;
        // printf("online vad return: %d\n", ret);  
        if (out_buf && out_len){
            memcpy(required_out+required_out_cur_len, out_buf, out_len);
            required_out_cur_len += out_len;
            free(out_buf); 
        }
    }


    unsigned char * out_buf = NULL;
    int  out_len = 0;
    online_vad_flush(online_vad_engine,  
                                &out_buf, 
                                &out_len);
    if (out_buf && out_len){
        memcpy(required_out+required_out_cur_len, out_buf, out_len);
        required_out_cur_len += out_len;
        free(out_buf);
    }

    printf("]\n");
    printf("stop\n");


    online_vad_engine_destory(online_vad_engine);

    printf("total input bytes: %d\n", bytes_used);
    printf("total output bytes: %d\n", required_out_cur_len);


    short * in_16 = (short *) inBuffer;
    short * out_16 = (short *) required_out;
    int ret_success = 1;
    for (int i=0; i<required_out_cur_len/2; i++){
        if (in_16[i] != out_16[i]){
            ret_success = 0;
            printf("sample point not equal, in: %d VS out: %d\n",in_16[i], out_16[i]);
        }
    }
    if (ret_success){
        printf("\n\n---return successfully---\n\n");
    }

    free(inBuffer);
    free(required_out);
    
}

int main(int argc, char *argv[])
{
    printf("WebRTC Oline Voice Activity Detector Test\n");
    if (argc < 2)
        return -1;
    char *in_file = argv[1];
    online_vad_test(in_file);
    printf("按任意键退出程序 \n");
    getchar();
    return 0;
}
