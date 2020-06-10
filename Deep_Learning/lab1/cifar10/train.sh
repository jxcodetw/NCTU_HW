export CUDA_VISIBLE_DEVICES=0
python3 train.py --cnn_type vanilla --num_layers 20  --train_dir vanilla20_train
python3 train.py --cnn_type vanilla --num_layers 56  --train_dir vanilla56_train
python3 train.py --cnn_type vanilla --num_layers 110 --train_dir vanilla110_train

python3 train.py --cnn_type resnet  --num_layers 20  --train_dir resnet20_train
python3 train.py --cnn_type resnet  --num_layers 56  --train_dir resnet56_train
python3 train.py --cnn_type resnet  --num_layers 110 --train_dir resnet110_train

python3 train.py --cnn_type resnet  --num_layers 20  --pre_act True --train_dir resnet20_preact_train
python3 train.py --cnn_type resnet  --num_layers 56  --pre_act True --train_dir resnet56_preact_train
python3 train.py --cnn_type resnet  --num_layers 110 --pre_act True --train_dir resnet110_preact_train
