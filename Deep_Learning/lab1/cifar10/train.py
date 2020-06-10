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
tf.app.flags.DEFINE_string('cnn_type', "vanilla", """ vanilla or resnet """)
tf.app.flags.DEFINE_integer('num_layers', 20, """ number of layers """)
tf.app.flags.DEFINE_boolean('pre_act', False, """ use preact block """)


def train(total_layers, total_epoches):
	with tf.Graph().as_default() as g:
		global_step = tf.train.get_or_create_global_step()
		num_batch_per_epoch = helper.batch_num(input_cifar10.NUM_EXAMPLES_PER_EPOCH_FOR_TRAIN, FLAGS.batch_size)
		num_batch_per_epoch_testing = helper.batch_num(input_cifar10.NUM_EXAMPLES_PER_EPOCH_FOR_TESTING, FLAGS.batch_size_testing)

		is_training = tf.placeholder(tf.bool, name='is_training')
		# read image
		with tf.device('/cpu:0'):
			images_train, labels_train = input_data(
				os.path.join(FLAGS.dataset_dir, 'cifar-10-batches-bin'),
				FLAGS.batch_size, test_data=False, shuffle=True)
			images_test, labels_test = input_data(
				os.path.join(FLAGS.dataset_dir, 'cifar-10-batches-bin'),
				FLAGS.batch_size_testing, test_data=True, shuffle=False)

			images = tf.cond(is_training, lambda: images_train, lambda: images_test)
			labels = tf.cond(is_training, lambda: labels_train, lambda: labels_test)

		elapsed_epoches = tf.div(tf.cast(global_step, tf.int32), tf.constant(num_batch_per_epoch, tf.int32))

		# net
		if FLAGS.cnn_type == "resnet":
			logits = resnet.inference(images, (total_layers-2) // 6, is_training, resnet=True)
		else:
			logits = resnet.inference(images, (total_layers-2) // 6, is_training, resnet=False)
		loss = resnet.loss(logits, labels)
		optimize = resnet.train(loss, elapsed_epoches, global_step)

		# summary
		summary_op = tf.summary.merge_all()

		top_k_op = tf.nn.in_top_k(logits, labels, 1)
		# counter
		true_count = tf.get_variable(dtype=tf.float32, shape=(), name='counter', initializer=tf.zeros_initializer())
		add_true_count = tf.assign(true_count, true_count + tf.reduce_sum(tf.cast(top_k_op, tf.float32)))
		zero_true_count = tf.assign(true_count, 0)

		# training error
		total_count = tf.constant(input_cifar10.NUM_EXAMPLES_PER_EPOCH_FOR_TRAIN, tf.float32)
		training_accuracy = 100.0 * true_count / total_count
		training_error = (100.0 - training_accuracy)
		training_accuracy_summary = tf.summary.scalar('training accuracy', training_accuracy)
		training_error_summary = tf.summary.scalar('training error', training_error)

		# testing error
		total_count_testing = tf.constant(input_cifar10.NUM_EXAMPLES_PER_EPOCH_FOR_TESTING, tf.float32)
		testing_accuracy = 100.0 * true_count / total_count_testing
		testing_error = (100.0 - testing_accuracy)
		testing_accuracy_summary = tf.summary.scalar('testing accuracy', testing_accuracy)
		testing_error_summary = tf.summary.scalar('testing error', testing_error)

		with tf.control_dependencies([optimize, add_true_count]):
			train_op = tf.no_op('train_op')

		with tf.control_dependencies([logits, add_true_count]):
			test_op = tf.no_op('test_op')

		# before training
		saver = tf.train.Saver()
		init_op = tf.global_variables_initializer()
		with tf.Session() as sess:
			sess.run([init_op])
			coord = tf.train.Coordinator()
			threads = tf.train.start_queue_runners(sess=sess, coord=coord)

			# save all summary

			summary_writer = tf.summary.FileWriter(FLAGS.train_dir, g)


			print('num_batch_per_epoch_for_training: ', num_batch_per_epoch)
			print('start training...')
			training_feed_dict = {
				is_training: True
			}
			testing_feed_dict = {
				is_training: False
			}
			last_training_error = '?'
			last_testing_error = '?'
			for ep in range(total_epoches):
				for bc in range(num_batch_per_epoch):
					_, gs, loss_val = sess.run([train_op, global_step, loss], feed_dict=training_feed_dict)
					print('\repoch={}/{}, batch={}/{}, traing/test error={}/{} loss={}'
						.format(ep+1, total_epoches, bc+1, num_batch_per_epoch, last_training_error, last_testing_error, loss_val), end='')
				summary_writer.add_summary(sess.run(summary_op, feed_dict=training_feed_dict), gs)

				#print('training error = %.3f' % (sess.run(training_error) * 100))
				last_training_error = '%.3f' % (sess.run(training_error))
				summary_writer.add_summary(sess.run(training_accuracy_summary, feed_dict=training_feed_dict), gs)
				summary_writer.add_summary(sess.run(training_error_summary, feed_dict=training_feed_dict), gs)
				sess.run(zero_true_count)

				for _ in range(num_batch_per_epoch_testing):
					_ = sess.run([test_op], feed_dict=testing_feed_dict)

				last_testing_error = '%.3f' % (sess.run(testing_error))
				summary_writer.add_summary(sess.run(testing_accuracy_summary, feed_dict=testing_feed_dict), gs)
				summary_writer.add_summary(sess.run(testing_error_summary, feed_dict=testing_feed_dict), gs)
				sess.run(zero_true_count)

				saver.save(sess, os.path.join(FLAGS.train_dir, 'model.ckpt'), gs)

			summary_writer.close()
			coord.request_stop()
			coord.join(threads)
			print()
			print('over')

def main(argv=None):
	tf.logging.set_verbosity(tf.logging.ERROR)
	print('='*30)
	for k in tf.flags.FLAGS.__flags:
		print('{} = {}'.format(k, tf.flags.FLAGS.__flags[k]))
	print('='*30)
	print('Will start training in ', end='')
	wait_sec = 5
	for i in range(wait_sec):
		print('{}s '.format(wait_sec-i), end='')
		sys.stdout.flush()
		time.sleep(1)
	print()
	#input('make sure all correct')

	run_config = tf.ConfigProto()
	run_config.operation_timeout_in_ms = 1000000
	dataset.download()
	if tf.gfile.Exists(FLAGS.train_dir):
		tf.gfile.DeleteRecursively(FLAGS.train_dir)
	tf.gfile.MakeDirs(FLAGS.train_dir)
	train(total_layers=FLAGS.num_layers, total_epoches=164)

if __name__ == "__main__":
	tf.app.run()
