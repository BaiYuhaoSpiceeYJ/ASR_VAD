from torch.autograd import Variable
from dataset import *
import argparse


def main(opts):
    batch_size = opts.batch_size
    slice_size = opts.slice_size
    noise_dataset = opts.noise_dataset
    noise_pickle_save_path = opts.noise_pickle_save_path
    voice_dataset = opts.voice_dataset
    voice_pickle_save_path = opts.voice_pickle_save_path
    ckpt_load_path = opts.ckpt_load_path

    # load data
    dset_noise = SEDataset(noise_dataset, 0.95, cache_dir=noise_pickle_save_path, split='test',
                           stride=1, slice_size=slice_size, max_samples=None, verbose=True,
                           slice_workers=4, preemph_norm=False, random_scale=[1], if_speak=False)
    dset_voice = SEDataset(voice_dataset, 0.95, cache_dir=voice_pickle_save_path, split='test',
                           stride=1, slice_size=slice_size, max_samples=None, verbose=True,
                           slice_workers=4, preemph_norm=False, random_scale=[1], if_speak=True)
    dloader_noise = DataLoader(dset_noise, batch_size=batch_size, shuffle=False, num_workers=0, collate_fn=collate_fn)
    dloader_voice = DataLoader(dset_voice, batch_size=batch_size, shuffle=False, num_workers=0, collate_fn=collate_fn)

    assert ckpt_load_path is not None
    msresnet = torch.load(ckpt_load_path)
    print('load', ckpt_load_path)
    msresnet = msresnet.cuda()
    msresnet.eval()

    correct_test = 0
    num_test_instances = len(dset_noise)
    for (bname_noise, samples_noise, labels_noise, idx_noise) in tqdm(dloader_noise):
        with torch.no_grad():
            samples_noise = samples_noise.unsqueeze(1)
            samplesV_noise = Variable(samples_noise.cuda())
            labels_noise = labels_noise.squeeze()
            labelsV_noise = Variable(labels_noise.cuda())
            # num_test_instances += labelsV_noise.size()[0]
            labelsV_noise = labelsV_noise.view(-1)

        predict_label = msresnet(samplesV_noise)
        # print(predict_label[0].data)
        prediction = predict_label[0].data.max(1)[1]
        # print(prediction)
        correct_test += prediction.eq(labelsV_noise.data.long()).sum()

    true_negative = float(correct_test)
    false_positive = float(num_test_instances - true_negative)
    print("Test noise accuracy:{}%".format(100 * float(correct_test) / num_test_instances))

    correct_test = 0
    num_test_instances = len(dset_voice)
    for (bname_voice, samples_voice, labels_voice, idx_voice) in tqdm(dloader_voice):
        with torch.no_grad():
            samples_voice = samples_voice.unsqueeze(1)
            samplesV_voice = Variable(samples_voice.cuda())
            labels_voice = labels_voice.squeeze()
            labelsV_voice = Variable(labels_voice.cuda())
            # num_test_instances += labelsV_voice.size()[0]
            labelsV_voice = labelsV_voice.view(-1)

        predict_label = msresnet(samplesV_voice)
        prediction = predict_label[0].data.max(1)[1]
        # print(prediction)
        correct_test += prediction.eq(labelsV_voice.data.long()).sum()

    true_positive = float(correct_test)
    false_negative = float(num_test_instances - true_positive)
    print("Test voice accuracy:{}%".format(100 * float(correct_test) / num_test_instances))

    precision = true_positive / (true_positive + false_positive)
    recall = true_positive / (true_positive + false_negative)
    accuracy = (true_positive + true_negative) / (true_positive + true_negative + false_negative + false_positive)
    f1 = 2 * precision * recall / (precision + recall)
    print('precision:{}%'.format(precision * 100))
    print('recall:{}%'.format(recall * 100))
    print('accuracy:{}%'.format(accuracy * 100))
    print('f1 score:{}%'.format(f1 * 100))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--batch_size', type=int, default=16)
    parser.add_argument('--slice_size', type=int, default=2**10)
    parser.add_argument('--noise_dataset', type=str, default=r'C:\Users\SpiceeYJ\Desktop\test0')
    parser.add_argument('--noise_pickle_save_path', type=str, default=r'C:\Users\SpiceeYJ\Desktop\test0\save_10')
    parser.add_argument('--voice_dataset', type=str, default=r'C:\Users\SpiceeYJ\Desktop\test1')
    parser.add_argument('--voice_pickle_save_path', type=str, default=r'C:\Users\SpiceeYJ\Desktop\test1\save_10')
    parser.add_argument('--ckpt_load_path', type=str, default=
                        r'C:\Users\SpiceeYJ\Desktop\Multi-Scale-1D-ResNet-master\weights2\4000.ckpt')
    opts = parser.parse_args()
    main(opts)

