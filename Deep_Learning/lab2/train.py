from __future__ import print_function
import argparse
import torch
import torch.nn as nn
#import torch.nn.functional as F
import torch.optim as optim
from torch.nn.modules import Sequential
from torchvision import datasets, transforms
from torch.autograd import Variable

from skimage.measure import compare_psnr
from PIL import Image
import PIL
import numpy as np
from tensorboardX import SummaryWriter

from downsampler import Downsampler

def str2bool(x):
	if x == 'False' or x == 'false' or x == '0':
		return False
	return True

# Training settings
parser = argparse.ArgumentParser(description='NCTU DeepLearning Lab2: Deep Image Prior')
parser.add_argument('--task', type=str, default="parameterization", metavar='Img', help='task')
parser.add_argument('--image', type=str, default="test_image.jpg", metavar='Img', help='target image')
parser.add_argument('--gt-image', type=str, default="noise_GT.png", metavar='Img', help='ground truth image')
parser.add_argument('--mask-image', type=str, default="noise_GT.png", metavar='Img', help='ground truth image')
parser.add_argument('--iterations', type=int, default=2400, metavar='N', help='number of epochs to train (default: 10)')
parser.add_argument('--factor', type=int, default=4, metavar='N', help='how many times bigger for SR')
parser.add_argument('--lr', type=float, default=0.01, help='learning rate')
parser.add_argument('--reg-noise-std', type=float, default=1./30., help='reg noise std')
parser.add_argument('--cuda', type=str2bool, default=False, help='enables CUDA training')
parser.add_argument('--psnr', type=str2bool, default=True, help='calculate psnr (need to have ground truth)')
parser.add_argument('--log-interval', type=int, default=100, metavar='N', help='how many batches to wait before logging training status')
args = parser.parse_args()

def act(act_fn='LeakyReLU'):
	if act_fn == 'LeakyReLU':
		return nn.LeakyReLU(0.2, inplace=True)
	assert False

def bn(num_features):
	return nn.BatchNorm2d(num_features)

def conv(in_f, out_f, ksize, stride=1, bias=True, pad='reflection', downsample='stride'):
	downsampler = None
	if stride != 1 and downsample != 'stride':
		print(stride, downsample)
		assert False, 'unsupported down sampling'

	padder = None
	to_pad = int((ksize - 1) / 2)
	if pad == 'reflection':
		padder = nn.ReflectionPad2d(to_pad)
		to_pad = 0

	convolver = nn.Conv2d(in_f, out_f, ksize, stride, padding=to_pad, bias=bias)

	layers = filter(lambda x: x is not None, [padder, convolver, downsampler])
	return nn.Sequential(*layers)

class DeepImagePrior(nn.Module):
	def __init__(self, input_depth=3, output_depth=3,
		ksize_down=[3, 3, 3, 3, 3], ksize_up = [3, 3, 3, 3, 3],
		ksize_skip=[1, 1, 1, 1, 1],
		f_down=[8, 16, 32, 64, 128], f_up=[8, 16, 32, 64, 128],
		f_skip=[0, 0, 0, 4, 4],
		#f_down=[128, 128, 128, 128, 128], f_up=[128, 128, 128, 128, 128],
		#f_skip=[4, 4, 4, 4, 4],
		upsample_mode='bilinear', downsample_mode='stride',
		need_bias = True,
		pad='reflection', act_fn='LeakyReLU'):
		super(DeepImagePrior, self).__init__()
		self.num_layers = len(f_down)
		self.downward = []
		self.skip = []
		self.skip_output = [None] * self.num_layers
		self.upward = []

		if isinstance(upsample_mode, str):
			upsample_mode = [upsample_mode] * self.num_layers
		if isinstance(downsample_mode, str):
			downsample_mode = [downsample_mode] * self.num_layers


		f_last = input_depth
		for i, _ in enumerate(f_down):
			d = Sequential(*[
				conv(f_last, f_down[i], ksize_down[i], 2, need_bias, pad, downsample_mode[i]),
				bn(f_down[i]),
				act(act_fn),

				conv(f_down[i], f_down[i], ksize_down[i], 1, need_bias, pad),
				bn(f_down[i]),
				act(act_fn)
			])
			self.downward.append(d)

			s = Sequential(*[
				conv(f_down[i], f_skip[i], ksize_skip[i], 1, need_bias, pad),
				bn(f_skip[i]),
				act(act_fn)
			])

			if f_skip[i] == 0:
				self.skip.append(None)
			else:
				self.skip.append(s)

			if i == self.num_layers - 1:
				f_in = f_down[i] + f_skip[i]
			else:
				f_in = f_up[i+1] + f_skip[i]
			f_out = f_up[i]
			u = Sequential(*[
				nn.Upsample(scale_factor=2, mode=upsample_mode[i]),
				bn(f_in),
				conv(f_in, f_out, ksize_up[i], 1, need_bias, pad),
				bn(f_out),
				act(act_fn),

				conv(f_out, f_out, 1, 1, need_bias, pad),
				bn(f_out),
				act(act_fn),
			])
			self.upward.append(u)
			f_last = f_down[i]

		self.last_layer = Sequential(*[
			conv(f_up[0], output_depth, 1, 1, need_bias, pad),
			nn.Sigmoid()
		])
		self.params = []
		for d in self.downward:
			self.params += list(d.parameters())
		for s in self.skip:
			if s != None:
				self.params += list(s.parameters())
		for u in self.upward:
			self.params += list(u.parameters())
		self.params += list(self.last_layer.parameters())

	def forward(self, x):
		for i in range(self.num_layers):
			x = self.downward[i](x)
			if self.skip[i] is not None:
				self.skip_output[i] = self.skip[i](x)
			else:
				self.skip_output[i] = None

		for i in reversed(range(self.num_layers)):
			if self.skip_output[i] is not None:
				x = torch.cat((x, self.skip_output[i]), 1)
			x = self.upward[i](x)
		return self.last_layer(x)

	def enable_cuda(self):
		self.cuda()
		for d in self.downward:
			d.cuda()
		for s in self.skip:
			if s != None:
				s.cuda()
		for u in self.upward:
			u.cuda()

def train(name, z, x0, x=None, mask=None, reg=True):
	writer = SummaryWriter('logs/'+name)

	if args.task == 'parameterization':
		model = DeepImagePrior(f_down=[8, 16, 32, 64, 128],
			f_up=[8, 16, 32, 64, 128],
			f_skip=[0, 0, 0, 4, 4])
		writer.add_image(name+'-parameterization/z', z[0], 1)
	elif args.task == 'denoise':
		model = DeepImagePrior(f_down=[128, 128, 128, 128, 128],
			f_up=[128, 128, 128, 128, 128],
			f_skip=[4, 4, 4, 4, 4])
	elif args.task == 'super-resolution':
		model = DeepImagePrior(f_down=[128, 128, 128, 128, 128],
			f_up=[128, 128, 128, 128, 128],
			f_skip=[4, 4, 4, 4, 4])
		downsampler = Downsampler(n_planes=3, factor=args.factor, kernel_type='lanczos2', phase=0.5, preserve_size=True).type(torch.cuda.FloatTensor)
	elif args.task == 'inpainting':
		model = DeepImagePrior(f_down=[8, 16, 32, 64, 128],
			f_up=[8, 16, 32, 64, 128],
			f_skip=[0, 0, 0, 0, 0],
			ksize_up = [5, 5, 5, 5, 5], upsample_mode='nearest')
		writer.add_image(name+'-inpainting/input/src', x0, 1)
		writer.add_image(name+'-inpainting/input/mask', mask, 1)
		x0 *= mask

	if args.cuda:
		model.enable_cuda()
	model.train()

	optimizer = optim.Adam(model.params, lr=args.lr)
	criterion = nn.MSELoss()
	if args.cuda:
		z, x0 = z.cuda(), x0.cuda()
		if mask is not None:
			mask = mask.cuda()
	z, x0 = Variable(z), Variable(x0)
	z_original = z.data.clone()
	noise = z.data.clone()

	best_psnr = -1
	for global_step in range(1, args.iterations+1):
		if reg:
			z.data = z_original + (noise.normal_() * args.reg_noise_std)

		optimizer.zero_grad()
		if args.task == 'super-resolution':
			output_hr = model(z)
			output = downsampler(output_hr)
		else:
			output = model(z)

		if args.task == 'inpainting':
			output_masked = output.clone()
			output_masked.data *= mask
			loss = criterion(output_masked, x0)
		else:
			loss = criterion(output, x0)
		loss.backward()
		optimizer.step()


		writer.add_scalar(args.task+'loss', loss.data[0], global_step)
		if args.task == 'denoise':
			if global_step % args.log_interval == 0 or global_step == 1:
				psnr = compare_psnr(x.numpy(), output.data[0].cpu().numpy())
				print("global step = {0}, loss = {1:.6f}, psnr={2:.6f}".format(global_step, loss.data[0], psnr))
				writer.add_scalar('psnr', psnr, global_step)
				if psnr > best_psnr:
					writer.add_image('image', output.data[0], global_step)
					best_psnr = psnr
		elif args.task == 'parameterization':
			if global_step % args.log_interval == 0 or global_step == 1:
				print("global step = {0}, loss = {1:.6f}".format(global_step, loss.data[0]))
				writer.add_image(name+'-reconstruction', output.data[0], global_step)
		elif args.task == 'super-resolution':
			if global_step % args.log_interval == 0 or global_step == 1:
				if args.psnr:
					psnr = compare_psnr(x.numpy(), output_hr.data[0].cpu().numpy())
					print("global step = {0}, loss = {1:.6f}, psnr={2:.6f}".format(global_step, loss.data[0], psnr))
					if psnr > best_psnr:
						writer.add_image('best_hr_image', output_hr.data[0], global_step)
						best_psnr = psnr
					writer.add_scalar('psnr', psnr, global_step)
				else:
					print("global step = {0}, loss = {1:.6f}".format(global_step, loss.data[0]))
				writer.add_image(name+'-hr', output_hr.data[0], global_step)
				writer.add_image(name+'-lr', output.data[0], global_step)
		elif args.task == 'inpainting':
			if global_step % args.log_interval == 0 or global_step == 1:
				print("global step = {0}, loss = {1:.6f}".format(global_step, loss.data[0]))
				writer.add_image(name+'-inpainting', output.data, global_step)

	writer.close()

def img_to_tensor(img, mask=False):
	img = np.array(img, dtype='float32')
	if mask and img.shape[-1] != 3:
		img = np.array([img, img, img]) / 255.
	else:
		img = img.transpose(2, 0, 1) / 255.
	img = torch.Tensor(img)
	return img

def get_data_denoise(path, gt_path, var=1./10):
	img_gt = img_to_tensor(Image.open(gt_path))
	img = img_to_tensor(Image.open(path))

	noise = torch.zeros(((1,) + img.shape))
	noise.uniform_()
	noise *= var
	return 'denoise', noise, img, img_gt

def get_data_super_resolution(path, gt_path, factor, var=1./10):
	img_hr = Image.open(gt_path)
	# div32
	new_size = (img_hr.size[0] - img_hr.size[0] % 32, img_hr.size[1] - img_hr.size[1] % 32)
	bbox = [
		(img_hr.size[0] - new_size[0])/2,
		(img_hr.size[1] - new_size[1])/2,
		(img_hr.size[0] + new_size[0])/2,
		(img_hr.size[1] + new_size[1])/2,
	]
	img_hr = img_hr.crop(bbox)
	img_lr = img_hr.resize([img_hr.size[0] // factor, img_hr.size[1] // factor], Image.ANTIALIAS)

	img_hr = img_to_tensor(img_hr)
	img_lr = img_to_tensor(img_lr)

	noise = torch.zeros(((1,) + img_hr.shape))
	noise.uniform_()
	noise *= var

	return 'super-resolution', noise, img_lr, img_hr

def get_data_super_resolution_nogt(path, factor, var=1./10):
	img_lr = Image.open(path)
	# div32
	new_size = (img_lr.size[0] - img_lr.size[0] % 32, img_lr.size[1] - img_lr.size[1] % 32)
	bbox = [
		(img_lr.size[0] - new_size[0])/2,
		(img_lr.size[1] - new_size[1])/2,
		(img_lr.size[0] + new_size[0])/2,
		(img_lr.size[1] + new_size[1])/2,
	]
	img_lr = img_lr.crop(bbox)
	img_hr = img_lr.resize([img_lr.size[0] * factor, img_lr.size[1] * factor], Image.ANTIALIAS)

	img_hr = img_to_tensor(img_hr)
	img_lr = img_to_tensor(img_lr)

	noise = torch.zeros(((1,) + img_hr.shape))
	noise.uniform_()
	noise *= var
	return 'super-resolution-nogt', noise, img_lr, img_hr

def get_data_parameterization(path, z_type='image', var=1./5):
	img = img_to_tensor(Image.open(path))

	noise = torch.zeros(((1,) + img.shape))
	noise.uniform_()
	noise *= var

	img_z = img.clone()
	if z_type == 'image-shuffle':
		print('shuffle')
		tmp_tensor = torch.Tensor(img_z.numpy().transpose(1, 2, 0).reshape(img.shape[1]*img.shape[2], 3))
		tmp_tensor = tmp_tensor[torch.randperm(tmp_tensor.shape[0])]
		img_z = torch.Tensor(tmp_tensor.numpy().reshape(img.shape[1], img.shape[2], 3).transpose(2, 0, 1))
	img_z = img_z[None, :, :, :]

	if z_type == 'image':
		return 'parameterization-image', img_z, img
	elif z_type == 'image+noise':
		img_z = img_z + noise
		return 'parameterization-image+noise', img_z, img
	elif z_type == 'image-shuffle':
		return 'parameterization-shuffle', img_z, img
	elif z_type == 'uniform':
		return 'parameterization-uniform', noise, img

def get_data_inpainting(path, mask_path, var=1./10):
	img = img_to_tensor(Image.open(path))
	mask = img_to_tensor(Image.open(mask_path), mask=True)

	noise = torch.zeros(((1,) + img.shape))
	noise.uniform_()
	noise *= var

	return 'inpainting', noise, img, None, mask

if __name__ == '__main__':
	if args.cuda:
		print("cuda enabled")
	else:
		print("cuda disabled")

	if args.psnr:
		print("psnr enable {}".format(args.psnr))

	if args.task == 'parameterization':
		train(*get_data_parameterization(args.image, z_type='image'), reg=False)
		train(*get_data_parameterization(args.image, z_type='image+noise'), reg=False)
		train(*get_data_parameterization(args.image, z_type='image-shuffle'), reg=False)
		train(*get_data_parameterization(args.image, z_type='uniform'), reg=False)
	elif args.task == 'denoise':
		train(*get_data_denoise(args.image, args.gt_image), reg=True)
	elif args.task == 'super-resolution':
		if args.psnr:
			train(*get_data_super_resolution(args.image, args.gt_image, args.factor), reg=True)
		else:
			train(*get_data_super_resolution_nogt(args.image, args.factor), reg=True)
	elif args.task == 'inpainting':
		train(*get_data_inpainting(args.image, args.mask_image), reg=True)
	print("over")
