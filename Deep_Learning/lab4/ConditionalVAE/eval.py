from __future__ import print_function
import os
import logging
import torch
from torch import optim
from torch.nn import functional as F
from torch.autograd import Variable
from torchvision import datasets, transforms

import matplotlib.pyplot as plt

from args import args, device
from model import VAE
from dataloader import mnist
from tensorboard_logger import SummaryWriter

args.summary_path = os.path.join(args.checkpoint_path, 'eval')
summary_writer = SummaryWriter(args, args.summary_path)

logger = logging.getLogger('eval')
logger.setLevel(logging.INFO)

_, input_combine, *_ = mnist(args)
model = VAE(args).to(device)
logger.info('Loading weights: {}'.format(args.model_weight_path))
model.load_state_dict(torch.load(args.model_weight_path))
model.eval()
logger.info('Model weights restored')

def random_sample():
	unit_mu = torch.zeros(1, 20)
	unit_std = torch.ones(1, 20)
	eps = torch.normal(unit_mu, unit_std)
	return eps * 0.5

if __name__ == '__main__':
	logger.info('Generating 0 to 9')
	rows = []
	for r in range(10):
		eps = random_sample().repeat(10, 1).to(device)
		c = torch.diag(torch.ones(10)).to(device)

		output = model.decoder(eps, c)
		output = torch.squeeze(output, 1)
		
		rows.append(torch.cat([o for o in output], dim=1))
	recons = torch.cat(rows, dim=0)
	logger.info('Writing to tensorboard')
	summary_writer.add_image_summary('reconstruct', recons.data, 0)

	#plt.imsave('eval.png', recons.data, format='png', cmap='gray')

	logger.info('Generating 0 to 9 (disentangle)')
	rows = []
	eps_ = random_sample()
	dim_to_change = 0
	eps_[0][dim_to_change] = -0.6 * 5
	for r in range(10):
		eps_[0][dim_to_change] += 0.6
		eps = eps_.repeat(10, 1).to(device)
		c = torch.diag(torch.ones(10)).to(device)

		output = model.decoder(eps, c)
		output = torch.squeeze(output, 1)
		
		rows.append(torch.cat([o for o in output], dim=1))
	recons = torch.cat(rows, dim=0)
	logger.info('Writing to tensorboard')
	summary_writer.add_image_summary('disentangle', recons.data, 0)
	# plt.imsave('disentangle.png', recons.data, format='png', cmap='gray')

	if len(args.num_seq) > 0:
		logger.info('Generating custom sequence of numbers')
		rows = []
		for i in args.num_seq:
			eps = random_sample().to(device)
			num = ord(i) - ord('0')
			assert num >= 0 and num <= 9, 'please provide a number sequence'
			
			c = torch.zeros(1, 10).to(device)
			c[0, num] = 1
			output = model.decoder(eps, c)

			rows.append(output[0][0])
		recons = torch.cat(rows, dim=1)
		logger.info('Writing to tensorboard')
		summary_writer.add_image_summary('generate_sequence', recons.data, 0)
		summary_writer.close()
		logger.info('Done')
