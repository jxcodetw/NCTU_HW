import matplotlib.pyplot as plt
import numpy as np

LOG_FOLDER = 'logs'

def visualize(folder, base):
    logs = []
    for i in range(1, 6):
        with open('{}/{}/{}'.format(LOG_FOLDER, folder, i), 'r') as f:
            logs.append(list(map(float, f.readlines())))
    logs = np.array(logs) - base
    plt.figure(figsize=(12, 9))
    plt.plot(np.arange(logs.shape[1]), np.mean(logs, axis=0))
    plt.savefig('{}/{}_fig.png'.format(LOG_FOLDER, folder), bbox_inches='tight')
    plt.close()

visualize('p3', 0)
visualize('p4', 1000)
visualize('p6', 0)
visualize('p7', 1000)