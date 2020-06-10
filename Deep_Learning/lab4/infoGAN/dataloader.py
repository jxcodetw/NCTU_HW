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
	labels = torch.load(args.training_label_path).type(torch.LongTensor)

	logger.info('preprocess images')
	imgs = imgs / 255.
	imgs = imgs.view(imgs.shape[0], 1, imgs.shape[1], imgs.shape[2])

	logger.info('preprocess labels')
	arange = torch.arange(imgs.shape[0]).type(torch.LongTensor)
	one_hot_labels = torch.zeros(imgs.shape[0], 10)
	one_hot_labels[arange, labels] = 1
	
	logger.info('saving preprocessed tensors')
	torch.save(one_hot_labels, args.one_hot_labels_path)
	torch.save(labels, args.labels_path)
	torch.save(imgs, args.imgs_path)

	logger.info('preprocess done.')


def mnist(args):
	if not os.path.exists(args.one_hot_labels_path) or \
		not os.path.exists(args.labels_path) or \
		not os.path.exists(args.imgs_path):
		preprocess(args)

	# load preprocessed data
	imgs = torch.load(args.imgs_path)
	labels = torch.load(args.labels_path)
	one_hot_labels = torch.load(args.one_hot_labels_path)

	# build dataset for loader
	mnist = list(zip(imgs, labels, one_hot_labels))

	logger.info('MNIST loaded')
	return torch.utils.data.DataLoader(mnist, batch_size=args.batch_size), imgs, one_hot_labels
