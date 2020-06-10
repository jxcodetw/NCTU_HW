import torch
from torch import nn, optim
from torch.nn import functional as F
from torch.nn.modules import Sequential
from torchvision import datasets, transforms
from torch.autograd import Variable

from collections import OrderedDict

class Generator(nn.Module):
	def __init__(self):
		super(Generator, self).__init__()
		self.g = Sequential(
			nn.ConvTranspose2d(64, 512, kernel_size=(1, 1), stride=(1, 1), bias=False),
			nn.BatchNorm2d(512, eps=1e-5, momentum=0.1, affine=True),
			nn.ReLU(inplace=True),
			nn.ConvTranspose2d(512, 256, kernel_size=(7, 7), stride=(1, 1), bias=False),
			nn.BatchNorm2d(256, eps=1e-5, momentum=0.1, affine=True),
			nn.ReLU(inplace=True),
			nn.ConvTranspose2d(256, 128, kernel_size=(4, 4), stride=(2, 2), padding=(1, 1), bias=False),
			nn.BatchNorm2d(128, eps=1e-5, momentum=0.1, affine=True),
			nn.ReLU(inplace=True),
			nn.ConvTranspose2d(128, 64, kernel_size=(4, 4), stride=(2, 2), padding=(1, 1), bias=False),
			nn.BatchNorm2d(64, eps=1e-5, momentum=0.1, affine=True),
			nn.ReLU(inplace=True),
			nn.ConvTranspose2d(64, 1, kernel_size=(3, 3), stride=(1, 1), padding=(1, 1), bias=False),
			nn.Tanh(),
		)

	def forward(self, x):
		return self.g(x)

# two head
class DiscriminatorAndQ(nn.Module):
	def __init__(self):
		super(DiscriminatorAndQ, self).__init__()

		# shared by discriminator and Q
		self.front = Sequential(
			nn.Conv2d(1, 64, kernel_size=(4, 4), stride=(2, 2), padding=(1, 1), bias=False),
			nn.LeakyReLU(0.2, inplace=True),
			nn.Conv2d(64, 128, kernel_size=(4, 4), stride=(2, 2), padding=(1, 1), bias=False),
			nn.BatchNorm2d(128, eps=1e-5, momentum=0.1, affine=True),
			nn.LeakyReLU(0.2, inplace=True),
			nn.Conv2d(128, 256, kernel_size=(4, 4), stride=(2, 2), padding=(1, 1), bias=False),
			nn.BatchNorm2d(256, eps=1e-5, momentum=0.1, affine=True),
			nn.LeakyReLU(0.2, inplace=True),
			nn.Conv2d(256, 512, kernel_size=(4, 4), stride=(2, 2), padding=(1, 1), bias=False),
			nn.BatchNorm2d(512, eps=1e-5, momentum=0.1, affine=True),
			nn.LeakyReLU(0.2, inplace=True),
		)

		self.d = Sequential(
			nn.Conv2d(512, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False),
			nn.Sigmoid(),
		)

		self.q = Sequential(
			nn.Linear(512, 100, bias=True),
			nn.ReLU(inplace=True),
			nn.Linear(100, 10, bias=True),
		)

	def D(self, img):
		out = self.front(img)
		out = self.d(out)
		out = out.view(-1, 1)
		return out

	def Q(self, img):
		out = self.front(img)
		out = out.view(-1, 512)
		out = self.q(out)
		return out

	def forward(self, img):
		pass