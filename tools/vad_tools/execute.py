import os
import glob

wav_list = glob.glob('/workspace/output/clean/*.wav')
for wav in wav_list:
    print(wav)
    x = './bin/audio_segment --drop-silence --conf-file conf/conf.json {} output'.format(wav)
    os.system(x)