import subprocess
import matplotlib as mpl
from matplotlib import pyplot as plt
import numpy as np
import cPickle

RUNS = 8
PHI = 2**-15

# do latex figures
text_width = 7.1413
golden_mean = (np.sqrt(5)-1.0)/2.0         # Aesthetic ratio
fig_width = text_width/3  # width in inches
fig_height = fig_width*golden_mean       # heighOt in inches
fig_size = [2*fig_width, 2*fig_height]



print "Figure size:", fig_size
params = {'backend': 'ps',
           'axes.labelsize': 2*10,
           'text.fontsize': 2*10,
           'legend.fontsize': 2*10,
           'xtick.labelsize': 2*8,
           'ytick.labelsize': 2*8,
           'text.usetex': True,
           'figure.figsize': fig_size}

mpl.rcParams.update(params)

dataset = raw_input(
    '''
    Please pick dataset from the following options:
    UCLA-UDP
    UCLA-TCP
    YouTube
    Zipf1
    CAIDA
    SanJose
    '''
    )

skew = None
target_file = None
if dataset == "UCLA-UDP":
    target_file = "../../data/UCLA_UDP_trace"
elif dataset == "UCLA-TCP":
    target_file = "../../data/UCLA_TCP_trace"
elif dataset == "YouTube":
    target_file = "../../data/YoutubeDS.txt"
elif dataset == "CAIDA":
    target_file = "../../data/equinix-chicago.dirA.20151217-125911.UTC.anon.trace"
elif dataset == "SanJose":
    target_file = "../../data/equinix-sanjose.dirA.20140619-125904.UTC.anon.trace"
elif dataset == "Zipf0.7":
    skew = 0.7
elif dataset == "Zipf1.3":
    skew = 1.3
elif dataset == "Zipf1":
    skew = 1.
else:
    skew = 1.
    print "Invalid Input! Doing Zipf1"

gammandices = np.arange(-4,5)
gammas = [2**x for x in gammandices]
fn = "../src/Release/hh-zipf.exe"


try:
    speeds = cPickle.load(open(
        "../results/Gammas {} {} Runs Single Run.pkl".format(dataset,
                                                  RUNS),
        'rb'))
except:
    speeds = []
    for run in xrange(RUNS):
        results = {}
        first_iteration = True
        for gamma in gammas:
            command = [fn,"-gamma", str(gamma), "-phi",str(PHI),
                       "-r","1"]
            if skew:
                command += ["-z",str(skew)]
            if target_file:
                command += ["-f",target_file]
            out = subprocess.check_output(command)
            lines = [x.split() for x in out.splitlines()[1:-1]]
            titles = lines[0][:4]
            # prepare initial dictionary
            if first_iteration:
                for title in titles:
                    results[title] = {}
                first_iteration = False
            # add results to relevant place in dictionary
            for i in xrange(1,len(titles)):
                title = titles[i]
                for line in lines[1:]:
                    algorithm = line[0]
                    results[title][algorithm] = results[title].get(
                        algorithm,[])+[float(line[i])]
            #print results
        speed = results["Updates/ms"]
        speeds.append(speed)

    cPickle.dump(speeds,
                 open("../results/Gammas {} {} Runs Single Run.pkl".format(dataset,RUNS),
                      'wb'))

# clean speeds from infs
def plot_speeds(speeds, dataset):
    speeds = [ speed for speed in speeds if sum([ np.inf in lists for lists in speed.values()])==0]
    print "Cleaned",RUNS-len(speeds), "RUNS"

    average_speeds = {}
    for algorithm in speeds[0].keys():
            average_speeds[algorithm] = [
                sum([speed[algorithm][i] for speed in speeds])/len(speeds)
                for i in xrange(len(speeds[0][algorithm]))]
    plt.plot(gammas, average_speeds["ALS"], "-D", label="IM-SUM")
    plt.plot(gammas, average_speeds["LS"],"-d",label="DIM-SUM")
    plt.xscale("log",basex=2)
    plt.xlabel("$\gamma$")
    plt.ylabel("Updates/ms")
    plt.legend(loc="center")
    plt.tight_layout()
    fig = plt.gcf()
    fig.canvas.set_window_title(
        "Gammas {} {} Runs Single Run".format(dataset,len(speeds)))
    plt.show()

plot_speeds(speeds,dataset)

def load_and_plot(dataset):
    speeds = cPickle.load(open(
        "../results/Gammas {} {} Runs Single Run.pkl".format(dataset,RUNS)))
    plot_speeds(speeds,dataset)
