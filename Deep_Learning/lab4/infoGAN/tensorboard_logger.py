from io import BytesIO
import matplotlib.pyplot as plt
try:
	import tensorflow as tf
except ImportError:
	print("Tensorflow not installed; No tensorboard logging.")
	tf = None

class SummaryWriter():
	def __init__(self, args, log_path):
		self.args = args
		if args.fresh_log:
			import shutil
			import os
			if os.path.exists(args.summary_path):
				shutil.rmtree(args.summary_path)
				os.mkdir(args.summary_path)
		
		self.summary_writer = tf and tf.summary.FileWriter(log_path)

	def add_scaler_summary(self, key, value, step):
		if tf is not None:
			summary = tf.Summary(value=[tf.Summary.Value(tag=key, simple_value=value)])
			self.summary_writer.add_summary(summary, step)
			
			if step % self.args.log_interval:
				self.summary_writer.flush()

	def add_image_summary(self, key, img, step):
		if tf is not None:
			b = BytesIO()
			plt.imsave(b, img, format='png', cmap='gray')
			img_sum = tf.Summary.Image(encoded_image_string=b.getvalue(), height=img.shape[0], width=img.shape[1])
			summary = tf.Summary(value=[tf.Summary.Value(tag=key, image=img_sum)])
			self.summary_writer.add_summary(summary, step)

			if step % self.args.log_interval:
				self.summary_writer.flush()

	def close(self):
		self.summary_writer.flush()