# preprocess mnist to desire formate
import os
import logging
import torch
import torch.utils.data

logger = logging.getLogger('dataloader')
logger.setLevel(logging.INFO)

def preprocess(args):
	logger.info('preprocess dataset')

	logger.info('load raw data')
	imgs = torch.load(args.training_img_path)
	imgs = imgs / 255.
	imgs = imgs.view(imgs.shape[0], 1, imgs.shape[1], imgs.shape[2])
	labels = torch.load(args.training_label_path).type(torch.LongTensor)

	arange = torch.arange(imgs.shape[0]).type(torch.LongTensor)

	logger.info('preprocess image')
	input_one_hot = torch.zeros(imgs.shape[0], 10, imgs.shape[2], imgs.shape[3])
	input_one_hot[arange, labels] = 1
	input_combine = torch.cat((imgs, input_one_hot), dim=1)

	logger.info('preprocess label')
	c = torch.zeros(imgs.shape[0], 10)
	c[arange, labels] = 1
	
	logger.info('saving preprocessed tensors')
	torch.save(input_combine, args.input_combine_path)
	torch.save(c, args.c_path)
	torch.save(imgs, args.imgs_path)

	logger.info('preprocess done.')


def mnist(args):
	if not os.path.exists(args.input_combine_path) or \
		not os.path.exists(args.c_path) or \
		not os.path.exists(args.imgs_path):
		preprocess(args)

	# load preprocessed data
	input_combine = torch.load(args.input_combine_path)
	c = torch.load(args.c_path)
	imgs = torch.load(args.imgs_path)

	# build dataset for loader
	mnist = list(zip(input_combine, c, imgs))

	logger.info('MNIST loaded')
	return torch.utils.data.DataLoader(mnist, batch_size=args.batch_size), input_combine, c, imgs
