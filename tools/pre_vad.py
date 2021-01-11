import os
import glob

def pre_vad(input_dir, output_dir):

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    wav_list = glob.glob(os.path.join(input_dir, '**/*.wav'), recursive=True)
    for wav in wav_list:
        wav_rel_path = os.path.relpath(wav, input_dir)
        wav_output_dir = os.path.join(output_dir, wav_rel_path.split('.')[0])

        if not os.path.exists(wav_output_dir):
            print('pre vad ', wav)
            os.makedirs(wav_output_dir)
            x = './tools/vad_tools/bin/audio_segment --drop-silence --conf-file ./tools/vad_tools/conf/conf.json {} {}'\
                .format(wav, wav_output_dir)
            os.system(x)
        else:
            print('pre vad already exist', wav)


if __name__ == '__main__':
    pre_vad(r'C:\Users\SpiceeYJ\Desktop\vad\dataset_origin\pure_voice', r'C:\Users\SpiceeYJ\Desktop\vad\dataset_for_train\pure_voice')
