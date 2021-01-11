import librosa
import argparse
from tools.resample import resample
from tools.pre_vad import pre_vad
from train_realtime_add import *


def main(opts):

    # pre_vad(opts.pure_voice_origin, opts.pure_voice_after_vad)
    #resample(opts.pure_voice_after_vad, opts.pure_voice_after_resample, norm=True, sr=opts.sr)
    #resample(opts.voice_with_noise_origin, opts.voice_with_noise_after_resample, norm=True, sr=opts.sr)
    #resample(opts.pure_noise_origin, opts.pure_noise_after_resample, norm=False, sr=opts.sr)
    #resample(opts.noise_with_voice_origin, opts.noise_with_voice_after_resample, norm=False, sr=opts.sr)

    args = (opts.batch_size, opts.num_epoch, opts.slice_size,
            opts.pure_voice_after_resample, opts.pure_voice_pickle_save_path,
            opts.voice_with_noise_after_resample, opts.voice_with_noise_pickle_save_path,
            opts.pure_noise_after_resample, opts.pure_noise_pickle_save_path,
            opts.noise_with_voice_after_resample, opts.noise_with_voice_pickle_save_path,
            opts.ckpt_load_path, opts.ckpt_save_path, opts.loss_log_path)

    train_realtime(*args)


if __name__ == '__main__':

    parser = argparse.ArgumentParser()

    ##########################################
    parser.add_argument('--pure_voice_origin', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_origin\pure_voice')
    parser.add_argument('--pure_voice_after_vad', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_for_train\pure_voice_after_vad')
    parser.add_argument('--pure_voice_after_resample', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_for_train\pure_voice')
    parser.add_argument('--pure_voice_pickle_save_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_for_train\pure_voice_pickle_for_train')
    ##########################################
    parser.add_argument('--voice_with_noise_origin', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_origin\voice_with_noise')
    parser.add_argument('--voice_with_noise_after_resample', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_for_train\voice_with_noise')
    parser.add_argument('--voice_with_noise_pickle_save_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_for_train\voice_with_noise_pickle_for_train')
    ##########################################
    parser.add_argument('--pure_noise_origin', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_origin\pure_noise')
    parser.add_argument('--pure_noise_after_resample', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_for_train\pure_noise')
    parser.add_argument('--pure_noise_pickle_save_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_for_train\pure_noise_pickle_for_train')
    ##########################################
    parser.add_argument('--noise_with_voice_origin', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_origin\noise_with_voice')
    parser.add_argument('--noise_with_voice_after_resample', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_for_train\noise_with_voice')
    parser.add_argument('--noise_with_voice_pickle_save_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\vad\dataset_for_train\noise_with_voice_pickle_for_train')
    ##########################################
    parser.add_argument('--batch_size', type=int, default=128)
    parser.add_argument('--num_epoch', type=int, default=10)
    parser.add_argument('--slice_size', type=int, default=2 ** 10)
    parser.add_argument('--sr', type=int, default=8000)
    parser.add_argument('--ckpt_load_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\Multi-Scale-1D-ResNet-master\weights\5500.ckpt')
    parser.add_argument('--ckpt_save_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\Multi-Scale-1D-ResNet-master\weights\\')
    parser.add_argument('--loss_log_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\log.txt')

    opts = parser.parse_args()
    main(opts)
