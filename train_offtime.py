from torch.autograd import Variable
from dataset import *
from multi_scale_ori import *
import argparse


def main(opts):
    batch_size = opts.batch_size//2
    num_iters = opts.num_iters
    slice_size = opts.slice_size
    noise_dataset = opts.noise_dataset
    noise_pickle_save_path = opts.noise_pickle_save_path
    voice_dataset = opts.voice_dataset
    voice_pickle_save_path = opts.voice_pickle_save_path
    ckpt_load_path = opts.ckpt_load_path
    ckpt_save_path = opts.ckpt_save_path
    loss_log_path = opts.loss_log_path

    # load data
    dset_noise = SEDataset(noise_dataset, 0.95, cache_dir=noise_pickle_save_path, split='train', stride=0.5,
                           slice_size=slice_size, max_samples=None, verbose=True, slice_workers=4, preemph_norm=False,
                           random_scale=[0.3, 0.5, 0.7, 0.8, 0.9, 1, 1.2, 1.5, 2.0], if_speak=False)
    dset_voice = SEDataset(voice_dataset, 0.95, cache_dir=voice_pickle_save_path, split='train', stride=0.5,
                           slice_size=slice_size, max_samples=None, verbose=True, slice_workers=4, preemph_norm=False,
                           random_scale=[0.2, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1, 1.2, 1.5], if_speak=True)
    dloader_noise = DataLoader(dset_noise, batch_size=batch_size, shuffle=True, num_workers=0, collate_fn=collate_fn)
    dloader_voice = DataLoader(dset_voice, batch_size=batch_size, shuffle=True, num_workers=0, collate_fn=collate_fn)
    iter_dloader_noise = iter(dloader_noise)
    iter_dloader_voice = iter(dloader_voice)

    # load net
    if ckpt_load_path is None:
        msresnet = MSResNet(input_channel=1, layers=[1, 1, 1, 1], num_classes=2)
    else:
        msresnet = torch.load(ckpt_load_path)
        print('load', ckpt_load_path)
    msresnet = msresnet.cuda()
    print("Total number of parameters is {}  ".format(sum(x.numel() for x in msresnet.parameters())))

    criterion = nn.CrossEntropyLoss(size_average=False).cuda()
    optimizer = torch.optim.Adam(msresnet.parameters(), lr=0.0001)
    # scheduler = torch.optim.lr_scheduler.MultiStepLR(optimizer, milestones=[50, 100, 150, 200, 250, 300], gamma=0.1)

    for num_iter in range(num_iters):
        msresnet.train()
        # scheduler.step()

        bname_noise, samples_noise, labels_noise, idx_noise = iter_dloader_noise.next()
        # samples: 2 * 1024
        # torch接受的输入：batch * channel * length 即2*1*1024
        samples_noise = samples_noise.unsqueeze(1)
        samplesV_noise = Variable(samples_noise.cuda())
        labels_noise = labels_noise.squeeze()
        labelsV_noise = Variable(labels_noise.cuda())

        bname_voice, samples_voice, labels_voice, idx_voice = iter_dloader_voice.next()
        samples_voice = samples_voice.unsqueeze(1)
        samplesV_voice = Variable(samples_voice.cuda())
        labels_voice = labels_voice.squeeze()
        labelsV_voice = Variable(labels_voice.cuda())

        samplesV = torch.cat((samplesV_voice, samplesV_noise), 0)
        labelsV = torch.cat((labelsV_voice, labelsV_noise), 0)

        shuffle_idx = np.random.permutation(samples_voice.size()[0]+samplesV_noise.size()[0])
        samplesV = samplesV[shuffle_idx]
        labelsV = labelsV[shuffle_idx]

        optimizer.zero_grad()
        predict_label = msresnet(samplesV)

        loss = criterion(predict_label[0], labelsV)
        loss.backward()
        optimizer.step()

        if loss_log_path is None:
            print('Iter:{}    Loss:{}'.format(num_iter, loss.item()/opts.batch_size))
        else:
            f = open(loss_log_path, 'a')
            print('Iter:{}    Loss:{}'.format(num_iter, loss.item()/opts.batch_size), file=f)

        if num_iter % 500 == 0 and num_iter > 0:
            torch.save(msresnet, ckpt_save_path + str(num_iter) + '.ckpt')
            print('save {}'.format(ckpt_save_path + str(num_iter) + '.ckpt'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--batch_size', type=int, default=64)
    parser.add_argument('--num_iters', type=int, default=1000000)
    parser.add_argument('--slice_size', type=int, default=2**10)

    parser.add_argument('--noise_dataset', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\noise-vad-8k-16bit')

    parser.add_argument('--noise_pickle_save_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\noise_pickle_for_train')

    parser.add_argument('--voice_dataset', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\added')

    parser.add_argument('--voice_pickle_save_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\voice_pickle_for_train')

    parser.add_argument('--ckpt_load_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\Multi-Scale-1D-ResNet-master\weights2\18500.ckpt')

    parser.add_argument('--ckpt_save_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\Multi-Scale-1D-ResNet-master\weights2\\')

    parser.add_argument('--loss_log_path', type=str,
                        default=r'C:\Users\SpiceeYJ\Desktop\log2.txt')

    opts = parser.parse_args()
    main(opts)
