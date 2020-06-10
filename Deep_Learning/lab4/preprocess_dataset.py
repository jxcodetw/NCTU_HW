import argparse
import gzip
import struct
import sys
import os
import torch
from urllib.request import urlretrieve

parser = argparse.ArgumentParser(description='Preprocess MNIST')
parser.add_argument('--src-folder', type=str, default="data", metavar='Src', help='where to save raw mnist dataset')
parser.add_argument('--dst-folder', type=str, default="data", metavar='Dst', help='where to save tensor')
args = parser.parse_args()

TRAINING_SET_IMAGES = 'http://yann.lecun.com/exdb/mnist/train-images-idx3-ubyte.gz'
TRAINING_SET_LABELS = 'http://yann.lecun.com/exdb/mnist/train-labels-idx1-ubyte.gz'

TEST_SET_IMAGES = 'http://yann.lecun.com/exdb/mnist/t10k-images-idx3-ubyte.gz'
TEST_SET_LABELS = 'http://yann.lecun.com/exdb/mnist/t10k-labels-idx1-ubyte.gz'

file_to_download = [
	TRAINING_SET_IMAGES,
	TRAINING_SET_LABELS,

	TEST_SET_IMAGES,
	TEST_SET_LABELS
]

def reporthook(blocknum, blocksize, totalsize):
	readsofar = blocknum * blocksize
	if totalsize > 0:
		percent = readsofar * 1e2 / totalsize
		percent = 100 if percent > 100 else percent

		sys.stderr.write('\r{0:.2f}%'.format(percent))
		if readsofar >= totalsize: # near the end
			sys.stderr.write("\n")
	else: # total size is unknown
		sys.stderr.write("read %d\n" % (readsofar,))

def url_to_filename(url):
	return os.path.join(args.src_folder, url.split('/')[-1])

def label_to_tensor(src, dst):
	print('Parsing label to', dst)
	dst = os.path.join(args.dst_folder, dst)

	f = gzip.open(src, 'rb')
	content = f.read()
	f.close()

	magic, num_label, = struct.unpack(">II", content[:2*4])
	assert magic == 0x00000801, 'Magic mismatch.'

	labels = struct.unpack(">{}".format('B' * num_label), content[8:])
	labels = torch.LongTensor(labels)
	torch.save(labels, dst)

def image_to_tensor(src, dst):
	print('Parsing image to', dst)
	dst = os.path.join(args.dst_folder, dst)

	f = gzip.open(src, 'rb')
	content = f.read()
	f.close()

	magic, num_image, rows, cols, = struct.unpack(">IIII", content[:4*4])
	assert magic == 0x00000803, 'Magic mismatch.'

	images = struct.unpack(">{}".format('B' * num_image * rows * cols), content[4*4:])
	images = torch.Tensor(images)
	images = images.view(-1, 28, 28)
	torch.save(images, dst)

def download():
	if not os.path.exists('data'):
		os.mkdir('data')
	for url in file_to_download:
		save_to = url_to_filename(url)
		if not os.path.exists(save_to):
			print("Downloading:", url)
			urlretrieve(url, save_to, reporthook)
		else:
			print("Downloaded. skip", url)

if __name__ == '__main__':
	if not os.path.exists(args.src_folder):
		os.mkdir(args.src_folder)
	if not os.path.exists(args.dst_folder):
		os.mkdir(args.dst_folder)

	download()
	
	image_to_tensor(url_to_filename(TRAINING_SET_IMAGES), 'training_images.pth')
	label_to_tensor(url_to_filename(TRAINING_SET_LABELS), 'training_labels.pth')
	image_to_tensor(url_to_filename(TEST_SET_IMAGES), 'test_images.pth')
	label_to_tensor(url_to_filename(TEST_SET_LABELS), 'test_labels.pth')
