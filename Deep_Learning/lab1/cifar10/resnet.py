from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import tensorflow as tf

FLAGS = tf.app.flags.FLAGS

def _variable_with_weight_decay(name, shape, stddev, wd):
	initializer = tf.contrib.layers.variance_scaling_initializer(factor=2.0, mode='FAN_IN', uniform=False) ###
	var = tf.get_variable(name, shape, initializer=initializer)

	if wd is not None:
		weight_decay = tf.multiply(tf.nn.l2_loss(var), wd, name='weight_loss')
		tf.add_to_collection('losses', weight_decay)
	return var

def _variable_on_cpu(name, shape, initializer):
	"""Helper to create a Variable stored on CPU memory."""
	with tf.device('/cpu:0'):
		initializer = tf.contrib.layers.variance_scaling_initializer(factor=2.0, mode='FAN_IN', uniform=False) ###
		var = tf.get_variable(name, shape, initializer=initializer, dtype=tf.float32)
	return var

def batch_norm_fn(x, n_out, is_training):
	with tf.variable_scope('bn'):
		beta = tf.Variable(tf.constant(0.0, shape=[n_out]),
									name='beta', trainable=True)
		gamma = tf.Variable(tf.constant(1.0, shape=[n_out]),
									name='gamma', trainable=True)
		batch_mean, batch_var = tf.nn.moments(x, [0, 1, 2], name='moments')
		ema = tf.train.ExponentialMovingAverage(decay=0.5)

		def mean_var_with_update():
			ema_apply_op = ema.apply([batch_mean, batch_var])
			with tf.control_dependencies([ema_apply_op]):
				return tf.identity(batch_mean), tf.identity(batch_var)

		mean, var = tf.cond(is_training,
							mean_var_with_update,
							lambda: (ema.average(batch_mean), ema.average(batch_var)))
		normed = tf.nn.batch_normalization(x, mean, var, beta, gamma, 1e-5)
		return normed

def _vanilla_block(prefix, name, tensor, in_filter, out_filter, stride=1, training=None):
	with tf.variable_scope('conv-{}-{}'.format(prefix, name)) as scope:
		kernel = _variable_with_weight_decay('weights',
												shape=[3, 3, in_filter, out_filter],
												stddev=5e-2,
												wd=0.0001)
		conv = tf.nn.conv2d(tensor, kernel, [1, stride, stride, 1], padding='SAME')

		# add bias
		biases = _variable_on_cpu('biases', [out_filter], tf.constant_initializer(0.0))
		pre_activation = tf.nn.bias_add(conv, biases)

		# batch norm
		batch_normed = batch_norm_fn(pre_activation, out_filter, training)
		#batch_normed = tf.layers.batch_normalization(pre_activation, training=training)

		# relu
		activation = tf.nn.relu(batch_normed, name=scope.name)
	return activation

def _residual_block_pre_act(prefix, name, tensor, in_filter, out_filter, stride=1, training=None):
	with tf.variable_scope('residual_block-{}-{}'.format(prefix, name)):
		kernel1 = _variable_with_weight_decay('weights1',
												shape=[3, 3, in_filter, out_filter],
												stddev=5e-2,
												wd=0.0001)
		#res = tf.layers.batch_normalization(tensor, training=training, name='batch_norm1')
		res = batch_norm_fn(tensor, in_filter, training)
		res = tf.nn.relu(res, name='relu1')
		res = tf.nn.conv2d(res, kernel1, [1, stride, stride, 1], padding='SAME')

		kernel2 = _variable_with_weight_decay('weights2',
												shape=[3, 3, out_filter, out_filter],
												stddev=5e-2,
												wd=0.0001)
		#res = tf.layers.batch_normalization(res, training=training, name='batch_norm2')
		res = batch_norm_fn(res, out_filter, training)
		res = tf.nn.relu(res, name='relu2')
		res = tf.nn.conv2d(res, kernel2, [1, 1, 1, 1], padding='SAME')

		if in_filter != out_filter or stride != 1:
			# downsample
			kernel_d = _variable_with_weight_decay('weights_downsample',
												shape=[1, 1, in_filter, out_filter],
												stddev=5e-2,
												wd=0.0001)
			shortcut = tf.nn.conv2d(tensor, kernel_d, [1, stride, stride, 1], padding='SAME')
		else:
			shortcut = tensor
		res = tf.add(res, shortcut, name='addition')
	return res

def _residual_block(prefix, name, tensor, in_filter, out_filter, stride=1, training=None):
	with tf.variable_scope('residual_block-{}-{}'.format(prefix, name)):
		kernel1 = _variable_with_weight_decay('weights1',
												shape=[3, 3, in_filter, out_filter],
												stddev=5e-2,
												wd=0.0001)
		res = tf.nn.conv2d(tensor, kernel1, [1, stride, stride, 1], padding='SAME')
		#res = tf.layers.batch_normalization(res, training=training, name='batch_norm1')
		res = batch_norm_fn(res, out_filter, training)
		res = tf.nn.relu(res, name='relu1')

		kernel2 = _variable_with_weight_decay('weights2',
												shape=[3, 3, out_filter, out_filter],
												stddev=5e-2,
												wd=0.0001)
		res = tf.nn.conv2d(res, kernel2, [1, 1, 1, 1], padding='SAME')
		#res = tf.layers.batch_normalization(res, training=training, name='batch_norm2')
		res = batch_norm_fn(res, out_filter, training)

		if in_filter != out_filter or stride != 1:
			# downsample
			kernel_d = _variable_with_weight_decay('weights_downsample',
												shape=[1, 1, in_filter, out_filter],
												stddev=5e-2,
												wd=0.0001)
			shortcut = tf.nn.conv2d(tensor, kernel_d, [1, stride, stride, 1], padding='SAME')
			#shortcut = tf.layers.batch_normalization(shortcut, training=training, name='batch_norm_downsample')
			shortcut = batch_norm_fn(shortcut, out_filter, training)
		else:
			shortcut = tensor
		res = tf.add(res, shortcut, name='addition')
		res = tf.nn.relu(res, name='relu2')
	return res



def inference(images, n, training=None, resnet=True):
	#net = _vanilla_block('fst', 'fst', images, 3, 16, 1, training)
	kernel_init = _variable_with_weight_decay('init_conv',
												shape=[3, 3, 3, 16],
												stddev=5e-2,
												wd=0.0001)
	net = tf.nn.conv2d(images, kernel_init, [1, 1, 1, 1], padding='SAME')
	if not resnet:
		print('Building Vanilla CNN')
		for i in range(2*n):
			net = _vanilla_block('32', i, net, 16, 16, 1, training)
		net = _vanilla_block('from32', 'to16', net, 16, 32, 2, training)
		for i in range(2*n-1):
			net = _vanilla_block('16', i, net, 32, 32, 1, training)
		net = _vanilla_block('from16', 'to8', net, 32, 64, 2, training)
		for i in range(2*n-1):
			net = _vanilla_block('8', i, net, 64, 64, 1, training)
	else:
		print('Building Resnet ', end='')
		if FLAGS.pre_act:
			print('preact')
			res_block = _residual_block_pre_act
		else:
			print('normal')
			res_block = _residual_block
		for i in range(n):
			net = res_block('32', i, net, 16, 16, 1, training)
		net = res_block('from32', 'to16', net, 16, 32, 2, training)
		for i in range(n-1):
			net = res_block('16', i, net, 32, 32, 1, training)
		net = res_block('from16', 'to8', net, 32, 64, 2, training)
		for i in range(n-1):
			net = res_block('8', i, net, 64, 64, 1, training)

		if FLAGS.pre_act:
			print('add last layer of BN ReLU')
			tf.summary.histogram('before_final_bn', net)
			net = batch_norm_fn(net, 64, training)
			tf.summary.histogram('after_final_bn', net)
			net = tf.nn.relu(net, name='relu_final')



	with tf.variable_scope('tail'):
		# avg pooling
		# before: (batch_size, 8, 8, 64)
		# after : (batch_size, 64)
		net = tf.reduce_mean(net, reduction_indices=[1, 2], name="avg_pooling")
		# Compute logits (1 per class) and compute loss.
		logits = tf.layers.dense(net, 10, activation=None, name='fc')
	return logits

def loss(logits, labels):
	labels = tf.cast(labels, tf.int64)

	y_label = tf.one_hot(labels, 10)
	y_ls = tf.nn.log_softmax(logits)
	cross_entropy_mean = tf.subtract(tf.constant(0, tf.float32), tf.reduce_mean(tf.reduce_sum(y_label*y_ls, reduction_indices=[1])))
	#cross_entropy = tf.nn.sparse_softmax_cross_entropy_with_logits(
	#	labels=labels, logits=logits, name='cross_entropy_per_example')
	#cross_entropy_mean = tf.reduce_mean(cross_entropy, name='cross_entropy')

	tf.summary.scalar('cross_entropy_mean', cross_entropy_mean)

	tf.add_to_collection('losses', cross_entropy_mean)
	return tf.add_n(tf.get_collection('losses'), name='total_loss')

def train(total_loss, epoch, global_step):
	boundaries = [81, 122]
	values = [0.1, 0.01, 0.001]
	lr = tf.train.piecewise_constant(epoch, boundaries, values)

	tf.summary.scalar('learning_rate', lr)
	with tf.control_dependencies([total_loss, lr]):
		opt = tf.train.MomentumOptimizer(lr, 0.9)
		#opt = tf.train.AdamOptimizer(learning_rate=lr)

		#gradients, variables = zip(*opt.compute_gradients(total_loss))
		#gradients, _ = tf.clip_by_global_norm(gradients, 100.0)
		#apply_gradient_op = opt.apply_gradients(zip(gradients, variables), global_step=global_step)
		gradients = opt.compute_gradients(total_loss)
		apply_gradient_op = opt.apply_gradients(gradients, global_step=global_step)



	with tf.control_dependencies([apply_gradient_op]):
		train_op = tf.no_op(name='train')

	return train_op
