from __future__ import print_function
import torch
from torch.utils.data.dataset import Dataset
from torch.utils.data.dataloader import default_collate
import os
import glob
import json
import gzip
import pickle
import timeit
import scipy.io.wavfile as wavfile
import numpy as np
import multiprocessing as mp
import random
import librosa
from torch.utils.data import DataLoader
import h5py
from tqdm import tqdm


def collate_fn(batch):
    # first we have utt bname, then tensors
    data_batch = []
    uttname_batch = []
    for sample in batch:
        uttname_batch.append(sample[0])
        data_batch.append(sample[1:])
    data_batch = default_collate(data_batch)
    return [uttname_batch] + data_batch

def slice_signal(signal, window_sizes, stride=0.5):
    """ Slice input signal

        # Arguments
            window_sizes: list with different sizes to be sliced
            stride: fraction of sliding window per window size

        # Returns
            A list of numpy matrices, each one being of different window size
    """
    assert signal.ndim == 1, signal.ndim
    n_samples = signal.shape[0]
    slices = []
    for window_size in window_sizes:
        offset = int(window_size * stride)
        slices.append([])
        for beg_i in range(n_samples + offset, offset):
            end_i = beg_i + offset
            if end_i > n_samples:
                # last slice is offset to past to fit full window
                beg_i = n_samples - offset
                end_i = n_samples
            slice_ = signal[beg_i:end_i]
            assert slice_.shape[0] == window_size, slice_.shape[0]
            slices[-1].append(slice_)
        slices[-1] = np.array(slices[-1], dtype=np.int32)
    return slices

def slice_index_helper(args):
    return slice_signal_index(*args)

def slice_signal_index(path, window_size, stride):
    """ Slice input signal into indexes (beg, end) each

        # Arguments
            window_size: size of each slice
            stride: fraction of sliding window per window size

        # Returns
            A list of tuples (beg, end) sample indexes
    """
    signal, rate = librosa.load(path, sr=None)
    assert stride <= 1, stride
    assert stride > 0, stride
    assert signal.ndim == 1, signal.ndim
    n_samples = signal.shape[0]
    slices = []
    offset = int(window_size * stride)
    #for beg_i in range(0, n_samples - (offset), offset):
    for beg_i in range(0, n_samples - window_size + 1, offset):
        end_i = beg_i + window_size
        # if end_i >= n_samples:
        #     #last slice is offset to past to fit full window
        #     beg_i = n_samples - window_size
        #     end_i = n_samples
        slice_ = (beg_i, end_i)
        slices.append(slice_)
    return slices

class SEDataset(Dataset):
    """ Speech enhancement dataset """
    def __init__(self, data_dir, preemph, cache_dir='.',
                 split='train', slice_size=2**10, if_speak=False,
                 stride=0.5, max_samples=None, verbose=False,
                 slice_workers=6, preemph_norm=False,
                 random_scale=[1]):
        super(SEDataset, self).__init__()
        print('Creating {} split out of data in {}'.format(split, data_dir))
        if data_dir.split('.')[-1] == 'scp':
            with open(data_dir, "r") as f:
                self.data_names = []
                for line in f.readlines():
                    self.data_names.append((line.strip('\n')).split('\t')[-1])
                f.close()
        else:
            self.data_names = sorted(glob.glob(os.path.join(data_dir, '**/*.wav'), recursive=True))

        print('Found {} names'.format(len(self.data_names)))
        self.slice_workers = slice_workers
        if len(self.data_names) == 0:
            raise ValueError('No wav data found! Check your data path please')
        if max_samples is not None:
            assert isinstance(max_samples, int), type(max_samples)
            self.data_names = self.data_names[:max_samples]

        # path to store pairs of wavs
        self.cache_dir = cache_dir
        self.slice_size = slice_size
        self.stride = stride
        self.split = split
        self.verbose = verbose
        self.preemph = preemph
        self.if_speak = if_speak
        # order is preemph + norm (rather than norm + preemph)
        self.preemph_norm = preemph_norm
        # random scaling list, selected per utterance
        self.random_scale = random_scale

        cache_path = cache_dir#os.path.join(cache_dir, '{}_chunks.pkl'.format(split))
        self.cache_path = cache_path

        if not os.path.exists(cache_path):
            os.makedirs(cache_path)

        self.prepare_slicing()
        print('Loaded {} idx2slice items'.format(len(self.idx2slice)))

    def read_wav_file(self, wavfilename):
        #rate, wav = wavfile.read(wavfilename)
        wav, rate = librosa.load(wavfilename, sr=8000)
        return rate, wav


    def prepare_slicing(self):
        """ Make a dictionary containing, for every wav file, its
            slices performed sequentially in steps of stride and
            sized slice_size
        """
        try:
            with open(os.path.join(self.cache_path, '{}_idx2slice.pkl'.format(self.split)), 'rb') as i2s_f:
                idx2slice = pickle.load(i2s_f)
        except :
            idx2slice = []
        print('prepare_slicing')

        for i in range(len(self.data_names)):
            data_path = self.data_names[i]

            if os.path.exists(os.path.join(self.cache_path, '{}_{}.pkl'.format(self.split, i))):
                print('{}/{}'.format(i, len(self.data_names)), data_path, 'pickle already dumped')
            else:
                slicing_i = []
                print('{}/{}'.format(i, len(self.data_names)), data_path, 'dump in pickle now')
                data_arg_i = (data_path, self.slice_size, self.stride)
                data_slices_i = slice_signal_index(*data_arg_i)

                for t_i, data_ss in enumerate(data_slices_i):
                    #if c_ss[1] - c_ss[0] < 1024:
                        # decimate less than 4096 samples window
                    #    continue
                    slicing_i.append({'data_slice': data_ss,
                                      'data_path': data_path,
                                      'slice_idx': t_i})
                    idx2slice.append((i, t_i))

                with open(os.path.join(self.cache_path, '{}_{}.pkl'.format(self.split, i)), 'wb') as ch_f:
                    pickle.dump(slicing_i, ch_f)
                with open(os.path.join(self.cache_path, '{}_idx2slice.pkl'.format(self.split)), 'wb') as i2s_f:
                    pickle.dump(idx2slice, i2s_f)

        self.idx2slice = idx2slice
        self.num_samples = len(self.idx2slice)
        self.slicings = None


    def extract_slice(self, index):
        # load slice
        s_i, e_i = self.idx2slice[index]
        #print('selected item: ', s_i, e_i)
        slice_file = os.path.join(self.cache_dir,
                                  '{}_{}.pkl'.format(self.split, s_i))
        #print('reading slice file: ', slice_file)
        with open(slice_file, 'rb') as s_f:
            slice_ = pickle.load(s_f)
            #print('slice_: ', slice_)
            slice_ = slice_[e_i]
            data_slice_ = slice_['data_slice']
            slice_idx = slice_['slice_idx']
            data_path = slice_['data_path']
            bname = os.path.splitext(os.path.basename(data_path))[0]
            met_path = os.path.join(os.path.dirname(data_path),
                                    bname + '.met')
            ssnr = None
            pesq = None
            if os.path.exists(met_path):
                metrics = json.load(open(met_path, 'r'))
                pesq = metrics['pesq']
                ssnr = metrics['ssnr']

            data_signal = self.read_wav_file(slice_['data_path'])[1]
            data_slice = data_signal[data_slice_[0]:data_slice_[1]]
            if data_slice.shape[0] < self.slice_size:
                pad_t = np.zeros((self.slice_size - data_slice.shape[0],))
                data_slice = np.concatenate((data_slice, pad_t))

            bname = os.path.splitext(os.path.basename(data_path))[0]
            return data_slice, pesq, ssnr, slice_idx, bname

    def __getitem__(self, index):
        data_slice, pesq, ssnr, slice_idx, bname = self.extract_slice(index)
        rscale = random.choice(self.random_scale)
        if rscale != 1:
            data_slice = rscale * data_slice

        label = [1] if self.if_speak else [0]
        returns = [bname, torch.FloatTensor(data_slice), torch.LongTensor(label), slice_idx]
        if pesq is not None:
            returns.append(torch.FloatTensor([pesq]))
        if ssnr is not None:
            returns.append(torch.FloatTensor([ssnr]))
        # print('idx: {} c_slice shape: {}'.format(index, c_slice.shape))
        return returns

    def __len__(self):
        return len(self.idx2slice)


