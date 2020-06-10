from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from six.moves import urllib
import os
import tarfile
import tensorflow as tf

FLAGS = tf.app.flags.FLAGS

tf.app.flags.DEFINE_string('dataset_dir', '/tmp/cifar10_data',
							"""Path to the CIFAR-10 dataset folder""")

def download(url='https://www.cs.toronto.edu/~kriz/cifar-10-binary.tar.gz'):
	"""Download the CIFAR-10 dataset if it haven't downloaded yet"""

	dataset_dir = FLAGS.dataset_dir
	if not os.path.exists(dataset_dir):
		print('created dataset_dir =', dataset_dir)
		os.mkdir(dataset_dir)

	filename = os.path.basename(url)
	filepath = os.path.join(dataset_dir, filename)

	if not os.path.exists(filepath):
		def _progress(count, block_size, total_size):
			percentage = float(count * block_size) / float(total_size) * 100
			print('\r{} {:0,.2f}%'.format(filename, percentage), end='')

		filepath, _ = urllib.request.urlretrieve(url, filepath, _progress)
		print(' -> extracting ... -> ', end='')

	extract_path = os.path.join(dataset_dir, 'cifar10-bin')
	if not os.path.exists(extract_path):
		tarfile.open(filepath, 'r:gz').extractall(dataset_dir)

	print('cifar10 downloaded.')


if __name__ == '__main__':
	download()