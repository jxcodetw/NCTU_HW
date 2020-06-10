import numpy as np
import matplotlib.pyplot as plt

X = np.array([ [0, 0], [0, 1], [1, 0], [1, 1] ])
Y = np.array([ [0], [1], [1], [0] ])

ITERATION = 5000

def sigmoid(x):
	return 1.0/(1.0 + np.exp(-x))

def dsigmoid(y):
	return y * (1 - y)

class Network:
	def __init__(self, layers, lr, M, seed=8888, reg = 1e-5):
		np.random.seed(seed)
		self.layers = layers
		self.lr = lr
		self.M = M
		self.reg = reg

		self.z = []
		self.dz = []
		self.a = []
		self.da = []
		self.w = [None]
		self.dw = [None]
		self.mdw = [None]
		self.b = [None]
		self.db = [None]
		self.mdb = [None]
		for i, l in enumerate(layers):
			self.z.append(np.zeros((1, l)))
			self.dz.append(np.zeros((1, l)))
			self.a.append(np.zeros((1, l)))
			self.da.append(np.zeros((1, l)))
			if i < len(layers)-1:
				self.w.append(np.random.randn(l, layers[i+1]) * 0.5)
				self.dw.append(np.random.randn(l, layers[i+1]) * 0.5)
				self.mdw.append(np.zeros((l, layers[i+1])))
				self.b.append(np.random.randn(1) * 0.5)
				self.db.append(np.random.randn(1) * 0.5)
				self.mdb.append(np.zeros((1)))

	def forward(self, x):
		self.a[0] = x
		for i in range(1, len(self.layers)):
			self.z[i] = self.a[i-1] @ self.w[i] + self.b[i]
			self.a[i] = sigmoid(self.z[i])
		return self.a[-1]

	def backprop(self, y, output):
		self.da[-1] = y - output
		for i in reversed(range(1, len(self.layers))):
			self.dz[i] = dsigmoid(self.a[i]) * self.da[i]
			self.dw[i] = self.a[i-1].T @ self.dz[i]
			self.db[i] = np.sum(np.sum(self.dz[i], axis=1), axis=0)
			self.da[i-1] = self.dz[i] @ self.w[i].T
			# clip
			np.clip(self.dw[i], -2, 2)
			np.clip(self.db[i], -2, 2)
			# weight update
			self.mdw[i] = self.M * self.mdw[i] + self.lr * self.dw[i] - 0.5 * self.reg * self.dw[i] * self.dw[i]
			self.mdb[i] = self.M * self.mdb[i] + self.lr * self.db[i] - 0.5 * self.reg * self.db[i] * self.db[i]
			self.w[i] += self.mdw[i]
			self.b[i] += self.mdb[i]

	def train(self, X, Y, epoch, verbose=True):
		step = epoch // 10
		history = []
		for i in range(epoch+1):
			out = self.forward(X)
			self.backprop(Y, self.a[-1])
			loss = np.mean(np.abs(Y - out))
			history.append(loss)
			if verbose and i % step == 0:
				print("iter: {:4}, loss: {}, output: {}".format(i, loss, out.flatten()))
		return history

def test(label, layers, lr, M, seed=8888, iter=ITERATION):
	n = Network(layers, lr, M, seed)
	return [n.train(X, Y, iter), label]

def plot(filename, logs):
	plt.title(filename)
	plt.xlabel('Iterations')
	plt.ylabel('Loss')
	plt.figure(figsize=(8, 6))
	lines = []
	for history, label in logs:
		line, = plt.plot(np.arange(len(history)), np.array(history), label=label)
		lines.append(line)
	plt.legend(handles=lines)
	plt.savefig('report/{}.png'.format(filename), bbox_inches='tight')
	plt.close()

def do_experiment(exp_name, setups):
	logs = []
	for setup in setups:
		logs.append(test(*setup))
	plot(exp_name, logs)

def exp_lr():
	layers = [2, 3, 1]
	setups = [["lr={}".format(lr/10), layers, lr/10, 0] for lr in range(1, 10)]
	do_experiment('loss_lr', setups)

def exp_momentum():
	layers = [2, 3, 3, 1]
	setups = [["momentum={}".format(m/10), layers, 0.8, m/10] for m in range(1, 10)]
	do_experiment('loss_momentum', setups)

def exp_seed():
	for hidden_size in range(2, 5):
		layers = [2, hidden_size, 1]
		setups = [["seed={}".format(seed*100), layers, 0.8, 0, seed*100] for seed in range(1, 10)]
		do_experiment('loss_{}_seed'.format(hidden_size), setups)

def exp_layer():
	setups = []
	for hidden_depth in reversed(range(1, 6)):
		layers = [2]
		for _ in range(hidden_depth):
			layers.append(6)
		layers.append(1)
		setups.append(['depth={}'.format(hidden_depth), layers, 0.8, 0.9, 9487, 5000])
	do_experiment('loss_depth', setups)

def main():
	experiments = [exp_lr, exp_momentum, exp_seed, exp_layer]
	for e in experiments:
		e()

if __name__ == '__main__':
	main()
