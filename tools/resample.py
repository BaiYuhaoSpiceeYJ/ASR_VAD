import librosa
import os
import glob

def resample(input_dir, output_dir, norm, sr=8000):

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    wav_list = glob.glob(os.path.join(input_dir, '**/*.wav'), recursive=True)
    for wav in wav_list:
        wav_rel_path = os.path.relpath(wav, input_dir)
        wav_output_name = os.path.join(output_dir, wav_rel_path)
        output_path = os.path.dirname(wav_output_name)

        if not os.path.exists(output_path):
            os.makedirs(output_path)

        if not os.path.exists(wav_output_name):
            print('resample ', wav_output_name)
            wave, sr_ = librosa.load(wav, sr=None)
            librosa.output.write_wav(wav_output_name, wave, sr=sr, norm=norm)
        else:
            print('resample already exist', wav_output_name)


if __name__ == '__main__':
    resample('a', 'b', True, sr=8000)
