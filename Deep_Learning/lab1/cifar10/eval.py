from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
import os
import sys
import time
import tensorflow as tf
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'

import helper
import dataset
import resnet
import input_cifar10
from input_cifar10 import input_data

FLAGS = tf.app.flags.FLAGS
tf.app.flags.DEFINE_integer('batch_size', 128, """ num examples per batch """)
tf.app.flags.DEFINE_integer('batch_size_testing', 200, """ num examples per batch for testing """)
tf.app.flags.DEFINE_string('train_dir', '/tmp/resnet_train', """ where to save log checkpoints """)
tf.app.flags.DEFINE_string('eval_dir', '/tmp/resnet_eval', """ evaluation tmp folder """)
tf.app.flags.DEFINE_string('cnn_type', "vanilla", """ vanilla or resnet """)
tf.app.flags.DEFINE_integer('num_layers', 20, """ number of layers """)
tf.app.flags.DEFINE_boolean('pre_act', False, """ use preact block """)
tf.app.flags.DEFINE_integer('num_eval', 1, """ iteration of evaluation """)

def eval(total_layers, total_epoches):
	with tf.Graph().as_default() as g:
		num_batch_per_epoch_testing = helper.batch_num(input_cifar10.NUM_EXAMPLES_PER_EPOCH_FOR_TESTING, FLAGS.batch_size_testing)

		is_training = tf.placeholder(tf.bool, name='is_training')
		# read image
		with tf.device('/cpu:0'):
			images, labels = input_data(
				os.path.join(FLAGS.dataset_dir, 'cifar-10-batches-bin'),
				FLAGS.batch_size_testing, test_data=True, shuffle=False)

		# net
		if FLAGS.cnn_type == "resnet":
			logits = resnet.inference(images, (total_layers-2) // 6, is_training, resnet=True)
		else:
			logits = resnet.inference(images, (total_layers-2) // 6, is_training, resnet=False)

		top_k_op = tf.nn.in_top_k(logits, labels, 1)

		# counter
		true_count = tf.get_variable(dtype=tf.float32, shape=(), name='counter', initializer=tf.zeros_initializer())
		add_true_count = tf.assign(true_count, true_count + tf.reduce_sum(tf.cast(top_k_op, tf.float32)))
		zero_true_count = tf.assign(true_count, 0)

		# testing error
		total_count_testing = tf.constant(input_cifar10.NUM_EXAMPLES_PER_EPOCH_FOR_TESTING, tf.float32)
		testing_accuracy = 100.0 * true_count / total_count_testing
		testing_error = (100.0 - testing_accuracy)

		with tf.control_dependencies([logits, add_true_count]):
			test_op = tf.no_op('test_op')

		# before training
		saver = tf.train.Saver()
		with tf.Session() as sess:
			saver.restore(sess, os.path.join(FLAGS.train_dir, "model.ckpt-64124"))
			print("Model restored")
			coord = tf.train.Coordinator()
			threads = tf.train.start_queue_runners(sess=sess, coord=coord)

			sess.run(zero_true_count)
			for _ in range(num_batch_per_epoch_testing):
				_ = sess.run([test_op], feed_dict={is_training: False})

			testing_error = '%.3f' % (sess.run(testing_error))
			print("test error = {}".format(testing_error))
			coord.request_stop()
			coord.join(threads)
			print('over')

def main(argv=None):
	tf.logging.set_verbosity(tf.logging.ERROR)
	print('='*30)
	for k in tf.flags.FLAGS.__flags:
		print('{} = {}'.format(k, tf.flags.FLAGS.__flags[k]))
	print('='*30)
	#input('make sure all correct')

	run_config = tf.ConfigProto()
	run_config.operation_timeout_in_ms = 1000000
	dataset.download()
	if tf.gfile.Exists(FLAGS.eval_dir):
		tf.gfile.DeleteRecursively(FLAGS.eval_dir)
	tf.gfile.MakeDirs(FLAGS.eval_dir)
	eval(total_layers=FLAGS.num_layers, total_epoches=FLAGS.num_eval)

if __name__ == "__main__":
	tf.app.run()
