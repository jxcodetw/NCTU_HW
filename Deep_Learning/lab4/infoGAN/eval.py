from __future__ import print_function
import os
import logging
import torch
from torch import nn, optim
from torch.autograd import Variable
from torchvision import datasets, transforms

import matplotlib.pyplot as plt

from args import args, device
from model import Generator, DiscriminatorAndQ
from dataloader import mnist
from tensorboard_logger import SummaryWriter

args.summary_path = os.path.join(args.checkpoint_path, 'eval')
summary_writer = SummaryWriter(args, args.summary_path)

logger = logging.getLogger('eval')
logger.setLevel(logging.INFO)

logger.info('Loading weights: {}'.format(args.netG_weight_path))
netG = Generator().to(device)
netG.load_state_dict(torch.load(args.netG_weight_path))
netG.eval()
logger.info('Generator loaded')

def sample_noise(batch_size):
	return torch.randn(batch_size, 54)

def evaluate(labels, noise=None):
	batch_size = len(labels)

	one_hot_labels = torch.zeros(batch_size, 10)
	one_hot_labels[range(batch_size), labels] = 1
	
	if noise is None:
		noise = sample_noise(batch_size)

	inputG = torch.cat((noise, one_hot_labels), dim=1)
	inputG = inputG.view(batch_size, 54+10, 1, 1)
	inputG = inputG.to(device)

	return netG(inputG)

if __name__ == '__main__':
	logger.info('eval start')
	rows = []
	for _ in range(10):
		fake_imgs = evaluate(range(10), sample_noise(10))
		fake_imgs = fake_imgs.squeeze(1)
		fake_imgs = torch.cat([img for img in fake_imgs], dim=1)
		rows.append(fake_imgs)

	fake_imgs = torch.cat(rows, dim=0).clamp(min=0)
	summary_writer.add_image_summary('image', fake_imgs.data, 0)

	# plt.imsave('eval.png', fake_imgs.data, format='png', cmap='gray')

	
	rows = []
	for _ in range(5):
		fake_imgs = evaluate([args.num_gen], sample_noise(1))
		fake_imgs = fake_imgs.squeeze(1)
		fake_imgs = torch.cat([img for img in fake_imgs], dim=1)
		rows.append(fake_imgs)

	fake_imgs = torch.cat(rows, dim=0).clamp(min=0)
	summary_writer.add_image_summary('demo', fake_imgs.data, 0)

	summary_writer.close()
	logger.info('Done')