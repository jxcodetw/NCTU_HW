from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os

import tensorflow as tf

IMAGE_SIZE = 32
NUM_EXAMPLES_PER_EPOCH_FOR_TRAIN = 50000
NUM_EXAMPLES_PER_EPOCH_FOR_TESTING = 10000

def read_a_cifar10_record(filename_queue):

	class CIFAR10Record(object):
		pass
	result = CIFAR10Record()

	result.width = 32
	result.height = 32
	result.depth = 3

	image_bytes = result.width * result.height * result.depth
	label_bytes = 1
	record_bytes = label_bytes + image_bytes

	reader = tf.FixedLengthRecordReader(record_bytes=record_bytes)
	result.key, value = reader.read(filename_queue)

	record_bytes = tf.decode_raw(value, tf.uint8)

	result.label = tf.cast(tf.strided_slice(record_bytes, [0], [label_bytes]), tf.int32)
	# The remaining bytes after the label represent the image
	image = tf.strided_slice(record_bytes, [label_bytes], [label_bytes + image_bytes])
	# reshape from [depth * height * width] to [depth, height, width].
	depth_major = tf.reshape(image, [result.depth, result.height, result.width])
	# Convert from [depth, height, width] to [height, width, depth].
	result.uint8image = tf.transpose(depth_major, [1, 2, 0])

	return result

def _augment(image):
	# padding
	paddings = tf.constant([[4, 4], [4, 4], [0, 0]])
	image_pad = tf.pad(image, paddings, "CONSTANT")

	# crop
	height = IMAGE_SIZE
	width = IMAGE_SIZE
	image_crop = tf.random_crop(image_pad, [height, width, 3])

	# flip
	image_flip = tf.image.random_flip_left_right(image_crop)
	return image_flip

def input_data(dataset_dir, batch_size, test_data=False, shuffle=True):
	""" construct input """
	if test_data:
		filenames = [os.path.join(dataset_dir, 'test_batch.bin')]
	else:
		filenames = [os.path.join(dataset_dir, 'data_batch_{}.bin'.format(i)) for i in range(1, 6)]
	for f in filenames:
		if not os.path.exists(f):
			raise ValueError('File: {} does not exist.'.format(f))
	filename_queue = tf.train.string_input_producer(filenames)
	with tf.name_scope('read_data'):
		# read a image
		record = read_a_cifar10_record(filename_queue)
		image = tf.cast(record.uint8image, tf.float32)

		if test_data:
			image_augmented = image
		else:
			image_augmented = _augment(image)

		# standarize
		image_std = tf.image.per_image_standardization(image_augmented)

		height = IMAGE_SIZE
		width = IMAGE_SIZE
		image_std.set_shape([height, width, 3])
		record.label.set_shape([1])

		min_fraction = 0.4
		if test_data:
			min_examples = int(NUM_EXAMPLES_PER_EPOCH_FOR_TESTING * min_fraction)
		else:
			min_examples = int(NUM_EXAMPLES_PER_EPOCH_FOR_TRAIN * min_fraction)

		num_preprocess_threads = 1
		if shuffle:
			images, labels = tf.train.shuffle_batch(
				[image_std, record.label],
				batch_size=batch_size,
				num_threads=num_preprocess_threads,
				capacity=min_examples + 3 * batch_size,
				min_after_dequeue=min_examples,
				allow_smaller_final_batch=True,
				name='training_input_queue'
			)
		else:
			images, labels = tf.train.batch(
				[image_std, record.label],
				batch_size=batch_size,
				num_threads=num_preprocess_threads,
				capacity=min_examples + 3 * batch_size,
				allow_smaller_final_batch=True,
				name='testing_input_queue'
			)
	return images, tf.reshape(labels, [batch_size])
