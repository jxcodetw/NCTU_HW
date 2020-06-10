from __future__ import print_function
import os
import logging
import numpy as np
import torch
from torch import nn, optim
from torch.autograd import Variable
from torchvision import datasets, transforms

from args import args, device
from model import Generator, DiscriminatorAndQ
from dataloader import mnist
from tensorboard_logger import SummaryWriter

args.summary_path = os.path.join(args.checkpoint_path, 'Generator')
summary_writerG = SummaryWriter(args, args.summary_path)
args.summary_path = os.path.join(args.checkpoint_path, 'Discriminator')
summary_writerD = SummaryWriter(args, args.summary_path)

logger = logging.getLogger('train')
logger.setLevel(logging.INFO)

def train(epochs):
	dataloader, *_ = mnist(args)
	netG = Generator().to(device)
	netDQ = DiscriminatorAndQ().to(device)

	netD = netDQ.D # Discriminator head
	netQ = netDQ.Q # Q head
	netG.train()
	netDQ.train()
	
	global_step = 0
	optimizerGQ = optim.Adam([{'params': netG.parameters()}, {'params': netDQ.q.parameters()}], lr=args.lr_generator, betas=(0.5, 0.99))
	optimizerD = optim.Adam([{'params': netDQ.front.parameters()}, {'params': netDQ.d.parameters()}], lr=args.lr_discriminator, betas=(0.5, 0.99))

	criterionD = nn.BCELoss()
	criterionQ = nn.CrossEntropyLoss()

	logger.info('Start training')
	for e in range(epochs):
		for batch_idx, (real_imgs, labels, one_hot_labels) in enumerate(dataloader):
			batch_size = real_imgs.shape[0]

			real_imgs, labels, one_hot_labels = real_imgs.to(device), labels.to(device), one_hot_labels.to(device)
			real = torch.ones(batch_size, 1).to(device)
			fake = torch.zeros(batch_size, 1).to(device)
			# combine noise and one hot vector
			inputG = torch.randn(batch_size, 54).to(device)
			inputG = torch.cat((inputG, one_hot_labels), dim=1)
			inputG = inputG.view(batch_size, 54+10, 1, 1)


			optimizerD.zero_grad()
			# train D with real
			d_out_real = netD(real_imgs)
			lossD_real = criterionD(d_out_real, real)
			# train D with fake
			fake_imgs = netG(inputG)
			d_out_fake = netD(fake_imgs.detach()) # update Discriminator only
			lossD_fake = criterionD(d_out_fake, fake)
			
			lossD = lossD_real + lossD_fake
			lossD.backward()
			optimizerD.step()
				
			# train G, Q
			optimizerGQ.zero_grad()
			d_out_fake = netD(fake_imgs)
			lossG = criterionD(d_out_fake, real)
			q_out = netQ(fake_imgs)
			lossQ = criterionQ(q_out, labels)
			lossGQ = lossG + lossQ
			lossGQ.backward()
			optimizerGQ.step()

			global_step += 1
			if global_step % args.log_interval == 0:
				logger.info('\repoch: {0}/{1}, lossD: {2:.5f}, lossG: {3:.5f}'
					.format(e+1, epochs, lossD.item(), lossG.item()))
				fake_imgs = fake_imgs.squeeze()
				fake_imgs = fake_imgs[:8]
				fake_imgs = torch.cat([o for o in fake_imgs], dim=1)
				summary_writerG.add_image_summary('image', fake_imgs.data, global_step)

			summary_writerG.add_scaler_summary('Loss', lossG.item(), global_step)
			summary_writerD.add_scaler_summary('Loss', lossD.item(), global_step)
		
		if e % args.checkpoint_epoch == 0:
			torch.save(netDQ.state_dict(), args.netDQ_weight_path)
			torch.save(netG.state_dict(), args.netG_weight_path)

	torch.save(netDQ.state_dict(), args.netDQ_weight_path)
	torch.save(netG.state_dict(), args.netG_weight_path)


if __name__ == '__main__':
	train(args.epochs)
	summary_writerG.close()
	summary_writerD.close()
	logger.info('Done')
