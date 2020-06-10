import logging
import argparse
import os
import torch

def parse_bool(val):
	if val is True or val == 'True' or val == 'true' or val == '1':
		return True
	return False

parser = argparse.ArgumentParser(description='VAE training')
parser.add_argument('--cuda', type=parse_bool, default=True, metavar='C', help='enable cuda')
parser.add_argument('--fresh-log', type=parse_bool, default=True, metavar='L', help='write fresh log')
parser.add_argument('--log-to-file', type=parse_bool, default=False, metavar='L', help='write log to file')
parser.add_argument('--epochs', type=int, default=100, metavar='E', help='total epochs')
parser.add_argument('--batch-size', type=int, default=128, metavar='B', help='batch size')
parser.add_argument('--lr', type=float, default=1e-3, metavar='LR', help='learning rate')
parser.add_argument('--noise-size', type=int, default=20, metavar='N', help='noise size')
parser.add_argument('--cond-code-size', type=int, default=10, metavar='C', help='condition code size')
parser.add_argument('--data-folder', type=str, default='../data', metavar='D', help='path to mnist dataset')
parser.add_argument('--feature-folder', type=str, default='feat', metavar='D', help='path to preprocessed feature')
parser.add_argument('--checkpoint-path', type=str, default='save', metavar='D', help='path to save model weight and logs')
parser.add_argument('--checkpoint-name', type=str, default='train', metavar='N', help='name of checkpoint')
parser.add_argument('--log-interval', type=int, default=200, metavar='L', help='how many batches to print')
parser.add_argument('--checkpoint-epoch', type=int, default=5, metavar='L', help='when to save a model')
parser.add_argument('--num-seq', type=str, default='', metavar='L', help='when to save a model')
args = parser.parse_args()

if args.log_to_file:
	args.logfile = os.path.join(args.checkpoint_path, 'log.txt')
else:
	args.logfile = None
logging.basicConfig(filename=args.logfile,level=logging.INFO)
logger = logging.getLogger('args')
logger.setLevel(logging.INFO)

# process arguments
if args.cuda:
	if torch.cuda.is_available():
		logger.info('Cuda Enabled')
	else:
		logger.warning('Cannot enable cuda!')
		args.cuda = False

args.training_img_path = os.path.join(args.data_folder, 'training_images.pth')
args.training_label_path = os.path.join(args.data_folder, 'training_labels.pth')
args.test_img_path = os.path.join(args.data_folder, 'test_images.pth')
args.test_label_path = os.path.join(args.data_folder, 'test_labels.pth')

args.input_combine_path = os.path.join(args.feature_folder, 'input_combine.pth')
args.c_path = os.path.join(args.feature_folder, 'c.pth')
args.imgs_path = os.path.join(args.feature_folder, 'imgs.pth')

if not os.path.exists(args.feature_folder):
	os.mkdir(args.feature_folder)

if not os.path.exists(args.checkpoint_path):
	os.mkdir(args.checkpoint_path)


args.model_weight_path = os.path.join(args.checkpoint_path, 'weight.pth')
args.summary_path = os.path.join(args.checkpoint_path, args.checkpoint_name)

device = torch.device("cuda" if args.cuda else "cpu")