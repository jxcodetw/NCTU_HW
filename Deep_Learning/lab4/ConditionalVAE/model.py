import torch
from torch import nn, optim
from torch.nn import functional as F
from torch.nn.modules import Sequential
from torchvision import datasets, transforms
from torch.autograd import Variable

class VAE(nn.Module):
	def __init__(self, args):
		super(VAE, self).__init__()
		self.args = args

		# encoder
		self.conv1 = Sequential(
			nn.Conv2d(11, 3, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1)),
			nn.ReLU(),
			nn.Conv2d(3, 1, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1)),
			nn.ReLU(),
		)
		self.fc1 = Sequential(
			nn.Linear(784, 400, bias=True),
			nn.ReLU(),
		)
		self.fc21 = nn.Linear(400, args.noise_size, bias=True)
		self.fc22 = nn.Linear(400, args.noise_size, bias=True)

		# decoder
		self.fc3 = Sequential(
			nn.Linear(10+args.noise_size, 392, bias=True),
			nn.ReLU(),
		)

		self.conv2 = Sequential(
			nn.Conv2d(2, 11, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1)),
			nn.ReLU(),
			nn.Upsample(scale_factor=2, mode='nearest'),
			nn.Conv2d(11, 3, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1)),
			nn.ReLU(),
			nn.Conv2d(3, 1, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1)),
			nn.Sigmoid(),
		)
	
	def encoder(self, x):
		x = self.conv1(x)
		x = x.view(-1, 784) # flatten
		x = self.fc1(x)
		mu = self.fc21(x)
		logvar = self.fc22(x)
		return mu, logvar

	def decoder(self, z, c):
		# (batch, 20), (batch, 10) -> (batch, 30)
		out = torch.cat((z, c), 1)
		out = self.fc3(out)
		
		out = out.view(-1, 2, 14, 14) # reshape
		out = self.conv2(out)
		return out

	def reparam(self, mu, logvar, eps=None):
		std = logvar.mul(0.5).exp()
		has_random = eps is not None
		
		if eps is None:
			unit_mu = torch.zeros(mu.shape)
			unit_std = torch.ones(std.shape)
			eps = torch.normal(unit_mu, unit_std)
		eps = Variable(eps)
		if self.args.cuda:
			eps = eps.cuda()

		if has_random:
			return eps
		return eps.mul(std).add(mu)

	def forward(self, x, c, eps=None):
		mu, logvar = self.encoder(x)
		z = self.reparam(mu, logvar, eps)
		return self.decoder(z, c), mu, logvar
