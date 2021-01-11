from torch.autograd import Variable
from dataset import *
from multi_scale_ori import *
import argparse


def train_realtime(batch_size, num_epoch, slice_size,
                   pure_voice_dataset, pure_voice_pickle_save_path,
                   voice_with_noise_dataset, voice_with_noise_pickle_save_path,
                   pure_noise_dataset, pure_noise_pickle_save_path,
                   noise_with_voice_dataset, noise_with_voice_pickle_save_path,
                   ckpt_load_path, ckpt_save_path, loss_log_path):

    """
    数据集分成四个部分，不含噪音的人声、含噪音的人声、不含人声的噪音和含人声的噪音，其中，前两者组成正样本，后两者组成负样本。
    在喂数据时，我们额外需要给不含噪音的人声按比例添加两种噪音。
    一个batch的组成为：
    50%正样本，50%负样本。
    其中50%的正样本又分为a%（不含噪音的人声+两种不同噪音）和(100-a)%（含噪音的人声组成）
    a%为不含噪音的人声数据集在正样本中的比例
    两种不同噪音的叠加比例为b%和（50-b）%
    50%的负样本分为b%（不含人声的噪音）和（100-b）%（含人声的噪音）
    b%为不含人声的噪音数据集在负样本中的比例
    注意：训练时要保证四个数据集分别至少含有0.25秒的音频
    """

    # load data
    dset_pure_noise = SEDataset(pure_noise_dataset, 0.95, cache_dir=pure_noise_pickle_save_path,
                                split='train', stride=0.5, slice_size=slice_size, max_samples=None,
                                verbose=True, slice_workers=4, preemph_norm=False,
                                random_scale=[0.4, 0.5, 0.7, 0.8, 0.9, 1.0], if_speak=False)

    dset_noise_with_voice = SEDataset(noise_with_voice_dataset, 0.95, cache_dir=noise_with_voice_pickle_save_path,
                                      split='train', stride=0.5, slice_size=slice_size, max_samples=None,
                                      verbose=True, slice_workers=4, preemph_norm=False,
                                      random_scale=[0.2, 0.4, 0.5, 0.6, 0.8], if_speak=False)

    dset_pure_voice = SEDataset(pure_voice_dataset, 0.95, cache_dir=pure_voice_pickle_save_path,
                                split='train', stride=0.5, slice_size=slice_size, max_samples=None,
                                verbose=True, slice_workers=4, preemph_norm=False,
                                random_scale=[0.5, 0.7, 0.8, 0.9, 1, 1.2, 1.5, 2.0], if_speak=True)

    dset_voice_with_noise = SEDataset(voice_with_noise_dataset, 0.95, cache_dir=voice_with_noise_pickle_save_path,
                                      split='train', stride=0.5, slice_size=slice_size, max_samples=None,
                                      verbose=True, slice_workers=4, preemph_norm=False,
                                      random_scale=[0.2, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1, 1.2, 1.5], if_speak=True)

    # calculate number
    num_pure_noise = len(dset_pure_noise)
    num_noise_with_voice = len(dset_noise_with_voice)
    num_pure_voice = len(dset_pure_voice)
    num_voice_with_noise = len(dset_voice_with_noise)

    num_negative = num_pure_noise + num_noise_with_voice
    num_positive = num_pure_voice + num_voice_with_noise
    num_total = num_negative + num_positive

    print('num_pure_noise', num_pure_noise)
    print('num_noise_with_voice', num_noise_with_voice)
    print('num_pure_voice', num_pure_voice)
    print('num_voice_with_noise', num_voice_with_noise)
    print('num_negative', num_negative)
    print('num_positive', num_positive)
    print('num_total', num_total)

    batch_size_pure_noise = max(int(batch_size / 2 * (num_pure_noise / num_negative)), 2)
    batch_size_noise_with_voice = max(int(batch_size / 2 - batch_size_pure_noise), 1)
    # batch_size_noise_with_voice = max((batch_size / 2 * (num_noise_with_voice / num_negative)), 2)
    batch_size_pure_voice = max(int(batch_size / 2 * (num_pure_voice / num_positive)), 2)
    batch_size_voice_with_noise = max(int(batch_size / 2 - batch_size_pure_voice), 1)
    # batch_size_voice_with_noise = max(int(batch_size / 2 * (num_voice_with_noise / num_positive)), 2)
    batch_size_pure_noise_for_add = max(int(batch_size_pure_voice * (num_pure_noise / num_negative)), 1)
    batch_size_noise_with_voice_for_add = max(batch_size_pure_voice - batch_size_pure_noise_for_add, 1)

    print('batch_size_pure_noise', batch_size_pure_noise)
    print('batch_size_noise_with_voice', batch_size_noise_with_voice)
    print('batch_size_pure_voice', batch_size_pure_voice)
    print('batch_size_voice_with_noise', batch_size_voice_with_noise)
    print('batch_size_pure_noise_for_add', batch_size_pure_noise_for_add)
    print('batch_size_noise_with_voice_for_add', batch_size_noise_with_voice_for_add)

    # load dataloader
    dloader_pure_noise = DataLoader(dset_pure_noise, batch_size=batch_size_pure_noise,
                                    shuffle=True, num_workers=0, collate_fn=collate_fn)

    dloader_noise_with_voice = DataLoader(dset_noise_with_voice, batch_size=batch_size_noise_with_voice,
                                          shuffle=True, num_workers=0, collate_fn=collate_fn)

    dloader_pure_noise_for_add = DataLoader(dset_pure_noise, batch_size=batch_size_pure_noise_for_add,
                                            shuffle=True, num_workers=0, collate_fn=collate_fn)

    dloader_noise_with_voice_for_add = DataLoader(dset_noise_with_voice, batch_size=batch_size_noise_with_voice_for_add,
                                                  shuffle=True, num_workers=0, collate_fn=collate_fn)

    dloader_pure_voice = DataLoader(dset_pure_voice, batch_size=batch_size_pure_voice,
                                    shuffle=True, num_workers=0, collate_fn=collate_fn)

    dloader_voice_with_noise = DataLoader(dset_voice_with_noise, batch_size=batch_size_voice_with_noise,
                                          shuffle=True, num_workers=0, collate_fn=collate_fn)

    # load iter
    iter_dloader_pure_noise = iter(dloader_pure_noise)
    iter_dloader_noise_with_voice = iter(dloader_noise_with_voice)
    iter_dloader_pure_voice = iter(dloader_pure_voice)
    iter_dloader_voice_with_noise = iter(dloader_voice_with_noise)
    iter_dloader_pure_noise_for_add = iter(dloader_pure_noise_for_add)
    iter_dloader_noise_with_voice_for_add = iter(dloader_noise_with_voice_for_add)

    # load net
    if ckpt_load_path is None:
        msresnet = MSResNet(input_channel=1, layers=[1, 1, 1, 1], num_classes=2)
    else:
        msresnet = torch.load(ckpt_load_path)
        print('load', ckpt_load_path)
    msresnet = msresnet.cuda()
    print("Total number of parameters is {}  ".format(sum(x.numel() for x in msresnet.parameters())))

    criterion = nn.CrossEntropyLoss(size_average=False).cuda()
    optimizer = torch.optim.Adam(msresnet.parameters(), lr=0.001)
    # scheduler = torch.optim.lr_scheduler.MultiStepLR(optimizer, milestones=[50, 100, 150, 200, 250, 300], gamma=0.1)
    num_iters = num_total // batch_size
    print('num iters:', num_iters)

    for epoch in range(num_epoch):
        for num_iter in range(num_iters):
            msresnet.train()
            # scheduler.step()

            bname_pure_noise, samples_pure_noise, labels_pure_noise, idx_pure_noise = \
                iter_dloader_pure_noise.next()
            samples_pure_noise = samples_pure_noise.unsqueeze(1)  # samples: 2 * 1024 ->batch * channel * length 即2*1*1024
            samplesV_pure_noise = Variable(samples_pure_noise.cuda())
            labels_pure_noise = labels_pure_noise.squeeze()
            labelsV_pure_noise = Variable(labels_pure_noise.cuda())

            bname_noise_with_voice, samples_noise_with_voice, labels_noise_with_voice, idx_noise_with_voice = \
                iter_dloader_noise_with_voice.next()
            samples_noise_with_voice = samples_noise_with_voice.unsqueeze(1)
            samplesV_noise_with_voice = Variable(samples_noise_with_voice.cuda())
            labels_noise_with_voice = labels_noise_with_voice.squeeze()
            labelsV_noise_with_voice = Variable(labels_noise_with_voice.cuda())

            bname_pure_voice, samples_pure_voice, labels_pure_voice, idx_pure_voice = \
                iter_dloader_pure_voice.next()
            samples_pure_voice = samples_pure_voice.unsqueeze(1)
            samplesV_pure_voice = Variable(samples_pure_voice.cuda())
            labels_pure_voice = labels_pure_voice.squeeze()
            labelsV_pure_voice = Variable(labels_pure_voice.cuda())

            bname_voice_with_noise, samples_voice_with_noise, labels_voice_with_noise, idx_voice_with_noise = \
                iter_dloader_voice_with_noise.next()
            samples_voice_with_noise = samples_voice_with_noise.unsqueeze(1)
            samplesV_voice_with_noise = Variable(samples_voice_with_noise.cuda())
            labels_voice_with_noise = labels_voice_with_noise.squeeze()
            labelsV_voice_with_noise = Variable(labels_voice_with_noise.cuda())

            bname_pure_noise_for_add, samples_pure_noise_for_add, labels_pure_noise_for_add, idx_pure_noise_for_add = \
                iter_dloader_pure_noise_for_add.next()
            samples_pure_noise_for_add = samples_pure_noise_for_add.unsqueeze(1)
            samplesV_pure_noise_for_add = Variable(samples_pure_noise_for_add.cuda())
            labels_pure_noise_for_add = labels_pure_noise_for_add.squeeze()
            labelsV_pure_noise_for_add = Variable(labels_pure_noise_for_add.cuda())

            bname_noise_with_voice_for_add, samples_noise_with_voice_for_add, labels_noise_with_voice_for_add, \
                idx_noise_with_voice_for_add = iter_dloader_noise_with_voice_for_add.next()
            samples_noise_with_voice_for_add = samples_noise_with_voice_for_add.unsqueeze(1)
            samplesV_noise_with_voice_for_add = Variable(samples_noise_with_voice_for_add.cuda())
            labels_noise_with_voice_for_add = labels_noise_with_voice_for_add.squeeze()
            labelsV_noise_with_voice_for_add = Variable(labels_noise_with_voice_for_add.cuda())

            samplesV_noise_for_add = torch.cat((samplesV_noise_with_voice_for_add, samplesV_pure_noise_for_add), 0)
            samplesV_pure_voice_added = torch.add(samplesV_noise_for_add, samplesV_pure_voice)

            samplesV = torch.cat((samplesV_pure_voice_added, samplesV_voice_with_noise,
                                 samplesV_pure_noise, samplesV_noise_with_voice), 0)
            labelsV = torch.cat((labelsV_pure_voice, labelsV_voice_with_noise,
                                 labelsV_pure_noise, labelsV_noise_with_voice), 0)

            shuffle_idx = np.random.permutation(samplesV.size()[0])
            samplesV = samplesV[shuffle_idx]
            labelsV = labelsV[shuffle_idx]

            optimizer.zero_grad()
            predict_label = msresnet(samplesV)

            loss = criterion(predict_label[0], labelsV)
            loss.backward()
            optimizer.step()

            if loss_log_path is None:
                print('Epoch:{}    Iter:{}    Loss:{}'.format(epoch, num_iter, loss.item()/samplesV.size()[0]))
            else:
                f = open(loss_log_path, 'a')
                print('Epoch:{}    Iter:{}    Loss:{}'.format(epoch, num_iter, loss.item()/samplesV.size()[0]), file=f)

            if num_iter % 100 == 0 and num_iter > 0:
                torch.save(msresnet, ckpt_save_path + str(epoch) + '_' + str(num_iter) + '.ckpt')
                print('save {}'.format(ckpt_save_path + str(epoch) + '_' + str(num_iter) + '.ckpt'))


if __name__ == '__main__':
    batch_size = 64
    num_epoch = 10
    slice_size = 2**10
    pure_voice_dataset = r'C:\Users\SpiceeYJ\Desktop\realtime\pure_voice'
    pure_voice_pickle_save_path = r'C:\Users\SpiceeYJ\Desktop\realtime\pure_voice_pickle_for_train'
    voice_with_noise_dataset = r'C:\Users\SpiceeYJ\Desktop\realtime\voice_with_noise'
    voice_with_noise_pickle_save_path = r'C:\Users\SpiceeYJ\Desktop\realtime\voice_with_noise_pickle_for_train'
    pure_noise_dataset = r'C:\Users\SpiceeYJ\Desktop\realtime\pure_noise'
    pure_noise_pickle_save_path = r'C:\Users\SpiceeYJ\Desktop\realtime\pure_noise_pickle_for_train'
    noise_with_voice_dataset = r'C:\Users\SpiceeYJ\Desktop\realtime\noise_with_voice'
    noise_with_voice_pickle_save_path = r'C:\Users\SpiceeYJ\Desktop\realtime\noise_with_voice_pickle_for_train'
    ckpt_load_path = r'C:\Users\SpiceeYJ\Desktop\Multi-Scale-1D-ResNet-master\weights\9000.ckpt'
    ckpt_save_path = r'C:\Users\SpiceeYJ\Desktop\Multi-Scale-1D-ResNet-master\weights\\'
    loss_log_path = None

    args = (batch_size, num_epoch, slice_size,
            pure_voice_dataset, pure_voice_pickle_save_path,
            voice_with_noise_dataset, voice_with_noise_pickle_save_path,
            pure_noise_dataset, pure_noise_pickle_save_path,
            noise_with_voice_dataset, noise_with_voice_pickle_save_path,
            ckpt_load_path, ckpt_save_path, loss_log_path)
    train_realtime(*args)
