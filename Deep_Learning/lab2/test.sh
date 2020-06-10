rm -rf logs
#python3 train.py --task denoise --cuda True --image noise_image.png --gt-image noise_GT.png --iterations 1800 --lr 0.01
#python3 train.py --task parameterization --cuda True --image test_image.png --iterations 2400 --lr 0.01
#python3 train.py --task super-resolution --cuda True --gt-image SR_GT.png --iterations 2000 --lr 0.01
python3 train.py --task super-resolution --cuda True --psnr False --factor 8 --image doge.png --iterations 10000 --lr 0.001
#python3 train.py --task inpainting --cuda True --image 2.png --mask-image 2_mask.png --iterations 5000 --lr 0.01
#python3 train.py --task inpainting --cuda True --image inpainting_2.jpg --mask-image inpainting_2_mask.jpg --iterations 10000 --lr 0.007