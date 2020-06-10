from __future__ import print_function
import os
import logging
import torch
from torch import optim
from torch.nn import functional as F
from torch.autograd import Variable
from torchvision import datasets, transforms

from args import args, device
from model import VAE
from dataloader import mnist
from tensorboard_logger import SummaryWriter

args.summary_path = os.path.join(args.checkpoint_path, 'train')
summary_writer = SummaryWriter(args, args.summary_path)

logger = logging.getLogger('train')
logger.setLevel(logging.INFO)

dataloader, *_ = mnist(args)
model = VAE(args).to(device)

def criterion(recon_x, x, mu, logvar):
	BCE = F.binary_cross_entropy(recon_x, x, size_average=False)
	KLD = -0.5 * torch.sum(1 + logvar - mu.pow(2) - logvar.exp())
	return BCE+KLD

def train(epochs):
	optimizer = optim.Adam(model.parameters(), lr=args.lr)
	global_step = 0
	model.train()

	logger.info('Start training')
	for e in range(epochs):
		for batch_idx, (input_combine, c, img) in enumerate(dataloader):
			input_combine, c, img = input_combine.to(device), c.to(device), img.to(device)

			optimizer.zero_grad()
			output, mu, logvar = model(input_combine, c)
			loss = criterion(output, img, mu, logvar)
			loss.backward()
			optimizer.step()

			global_step += 1
			if global_step % args.log_interval == 0:
				logger.info('\rloss: {0:.5f}, epoch: {1}/{2}'
					.format(loss.item() / args.batch_size, e+1, epochs))

			summary_writer.add_scaler_summary('Conditional VAE Loss', loss.item() / args.batch_size, global_step)
		
		if e % args.checkpoint_epoch == 0:
			torch.save(model.state_dict(), args.model_weight_path)
	torch.save(model.state_dict(), args.model_weight_path)


if __name__ == '__main__':
	train(args.epochs)
	summary_writer.close()
	logger.info('Done')