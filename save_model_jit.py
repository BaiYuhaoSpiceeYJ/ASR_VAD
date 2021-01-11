from train_realtime_add import *
import numpy as np
import random
import librosa
import timeit



# 用jit跟踪模型的流程，保存jit文件
def main(opts):

    # step1:载入模型
    msresnet = torch.load(opts.ckpt_load_path)
    msresnet.eval()
    # step2:构造一个输入example
    example = torch.rand(1, 1, 1024)

    if opts.device == "gpu":
        example = example.type(torch.cuda.FloatTensor)
        msresnet.to("cuda")
    else:
        msresnet.to("cpu")

    # step3:用jit跟踪模型数据流动
    traced_script_module = torch.jit.trace(msresnet, (example,))

    # step4:保存jit模型s
    traced_script_module.save(opts.save_path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--ckpt_load_path', type=str, default='./weights2/18500.ckpt')
    parser.add_argument('--save_path', type=str, default='./model-big.jit',
                        help='Path to save git-form model')
    parser.add_argument('--device', type=str, default="cpu")
    opts = parser.parse_args()
    main(opts)
