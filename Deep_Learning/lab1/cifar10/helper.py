import math

def batch_num(num_example_per_epoch, num_example_per_batch):
	return int(math.ceil(num_example_per_epoch / num_example_per_batch))
