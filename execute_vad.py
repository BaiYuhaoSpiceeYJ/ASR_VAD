from torch.autograd import Variable
from dataset import *
import librosa
import glob
import os
import numpy as np
import argparse


def main(opts):

    wav_dir = opts.wav_dir
    ckpt_load_path = opts.ckpt_load_path
    wav_list = glob.glob(os.path.join(wav_dir, '**/*.wav'), recursive=True)
    output_dir = opts.output_dir
    slice_size = opts.slice_size

    assert ckpt_load_path is not None
    msresnet = torch.load(ckpt_load_path)
    print('load', ckpt_load_path)
    msresnet = msresnet.cuda()
    msresnet.eval()

    for wav_path in wav_list:
        wav, sr = librosa.load(wav_path, sr=8000)
        vad = []
        wav_rel_path = os.path.relpath(wav_path, wav_dir)
        wav_output_dir = os.path.join(output_dir, wav_rel_path.split('.')[0])

        if not os.path.exists(wav_output_dir):
            os.makedirs(wav_output_dir)

        for slice in range(0, len(wav), slice_size):
            if len(wav) - slice >= slice_size:
                wav_segment = wav[slice:slice+slice_size]
            else:
                wav_segment = wav[slice:]
                wav_segment = np.pad(wav_segment, (0, slice_size - len(wav_segment)), constant_values=0)
            #wav_segment = np.ones(1024)

            wav_segment = torch.FloatTensor(wav_segment)   # 1024
            wav_segment = torch.unsqueeze(wav_segment, 0)  # 1 * 1024
            wav_segment = torch.unsqueeze(wav_segment, 0)  # 1 * 1 * 1024
            wav_segment = Variable(wav_segment.cuda())

            predict_label = msresnet(wav_segment)
            predict_label = predict_label[0].data.cpu().numpy()
            print(predict_label)
            label = np.argmax(predict_label)
            vad.append(label)

        print(wav_path)
        print(vad)

        buffer = [0, 0, 0]
        activate = False
        start = 0
        end = 0

        for i in range(len(vad)):
            buffer.pop(0)
            buffer.append(vad[i])

            if activate is False:
                if buffer == [0, 0, 0] or buffer == [0, 0, 1] or buffer == [0, 1, 0] or buffer == [1, 0, 0]:
                    continue
                elif buffer == [1, 0, 1]:
                    start = i - 3 if (i-3) >= 0 else 0
                    activate = True
                elif buffer == [0, 1, 1]:
                    start = i - 2 if (i-2) >= 0 else 0
                    activate = True
                else:  # buffer == [1, 1, 1] or buffer == [1, 1, 0]
                    print('error activate=False')
                    break
            else:
                if buffer == [1, 1, 1] or buffer == [1, 1, 0] or buffer == [1, 0, 1] or buffer == [0, 1, 1]:
                    if i == len(vad) - 1:
                        end = i
                        activate = False
                        wav_save_path = os.path.join(wav_output_dir,
                                                     os.path.basename(wav_path).split('.')[0] +
                                                     r'_{}_{}.wav'.format(start * slice_size / 8000,
                                                                          end * slice_size / 8000))
                        librosa.output.write_wav(wav_save_path, wav[start * slice_size:end * slice_size], sr=8000)
                    else:
                        continue
                elif buffer == [0, 1, 0]:
                    end = i
                    activate = False
                    wav_save_path = os.path.join(wav_output_dir,
                                                 os.path.basename(wav_path).split('.')[0] +
                                                 r'_{}_{}.wav'.format(start * slice_size / 8000,
                                                                      end * slice_size / 8000))
                    librosa.output.write_wav(wav_save_path, wav[start * slice_size:end * slice_size], sr=8000)
                elif buffer == [1, 0, 0]:
                    end = i 
                    activate = False
                    wav_save_path = os.path.join(wav_output_dir,
                                                 os.path.basename(wav_path).split('.')[0] +
                                                 r'_{}_{}.wav'.format(start * slice_size / 8000,
                                                                      end * slice_size / 8000))
                    librosa.output.write_wav(wav_save_path, wav[start * slice_size:end * slice_size], sr=8000)
                else:  # buffer == [0, 0, 0] or buffer == [0, 0, 1]
                    print('error activate=True')
                    break


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--slice_size', type=int, default=2**10)
    parser.add_argument('--wav_dir', type=str, default=r'C:\Users\SpiceeYJ\Desktop\test2')
    parser.add_argument('--ckpt_load_path', type=str, default=
                        r'C:\Users\SpiceeYJ\Desktop\Multi-Scale-1D-ResNet-master\weights\best.ckpt')
    parser.add_argument('--output_dir', type=str, default=r'C:\Users\SpiceeYJ\Desktop\test2_save')
    opts = parser.parse_args()
    main(opts)
