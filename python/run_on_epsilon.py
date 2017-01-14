import subprocess
import matplotlib as mpl
from matplotlib import pyplot as plt
from matplotlib import ticker
import numpy as np
import cPickle
RUNS = 8

def plot_speeds(speeds, dataset, legend = False):
    average_speeds = {}
    for algorithm in speeds[0].keys():
            average_speeds[algorithm] = [
                sum([speed[algorithm][i] for speed in speeds])/RUNS
                for i in xrange(len(speeds[0][algorithm]))]
    phindices = np.arange(21-len(average_speeds["ALS"]),21)
    phis = [2**(-x) for x in phindices]
    plt.plot(phis, average_speeds["ALS"], "-D", label="IM-SUM")
    plt.plot(phis, average_speeds["LS"],"-d",label="DIM-SUM")
    plt.plot(phis, average_speeds["SSH"],"-o",label="SSH")
    plt.plot(phis, average_speeds["CM"],"-+",label="CM")
    plt.plot(phis, average_speeds["CMH"],"-^",label="CMH")
    plt.plot(phis, average_speeds["CCFC"],"-v",label="AGT")
    
    plt.xscale("log",basex=2)
    plt.gca().xaxis.set_major_locator(ticker.LogLocator(base=4))
    plt.xlabel("$\epsilon$")
    plt.ylabel("Updates/ms")
    plt.xlim(left = 2**-20, right=2**-10)
    plt.ylim(ymin = 0)
    window_title = "{} {} Runs Gamma 4 Single Run".format(
        dataset,RUNS)
    if legend:
        plt.legend(loc="center")
        window_title = "epsilon_legend"
    plt.tight_layout()
    fig = plt.gcf()
    fig.canvas.set_window_title(window_title)


def load_and_plot(dataset):
    speeds = cPickle.load(
        open("../results/{} {} Runs Gamma 4 Single Run.pkl".format(dataset,
                                                        RUNS)))
    plot_speeds(speeds, dataset)
    

# do latex figures
text_width = 7.1413
golden_mean = (np.sqrt(5)-1.0)/2.0         # Aesthetic ratio
fig_width = text_width/3  # width in inches
fig_height = fig_width*golden_mean       # height in inches
fig_size = [2*fig_width, 2*fig_height]



print "Figure size:", fig_size
params = {'backend': 'ps',
           'axes.labelsize': 2*10,
           'text.fontsize': 2*10,
           'legend.fontsize': 2*7,
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

target_file = None
if dataset == "UCLA-UDP":
    target_file = "../../../data/UCLA_UDP_trace"
elif dataset == "UCLA-TCP":
    target_file = "../../../data/UCLA_TCP_trace"
elif dataset == "YouTube":
    target_file = "../../../data/YoutubeDS.txt"
elif dataset == "CAIDA":
    target_file = "../../../data/equinix-chicago.dirA.20151217-125911.UTC.anon.trace"
elif dataset == "SanJose":
    target_file = "../../../data/equinix-sanjose.dirA.20140619-125904.UTC.anon.trace"
elif dataset != "Zipf1":
    print "Invalid Input! Doing Zipf1"

show_legend = True if raw_input("Show legend? (Y/N)")=="Y" \
              else False


FIRST_PHINDEX = 10
phindices = range(FIRST_PHINDEX,21)
phis = [2**-x for x in phindices]
fn = "../src/Release/hh-zipf.exe"


try:
    speeds = cPickle.load(
        open("../results/{} {} Runs Gamma 4 Single Run.pkl".format(dataset,RUNS),'rb')
    )
except:
    speeds = []
    for run in xrange(RUNS):
        results = {}
        first_iteration = True
        for phi in phis:
            command = [fn,"-phi",str(phi),"-r","1"]
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
    cPickle.dump(
        speeds,
        open("../results/{} {} Runs Gamma 4 Single Run.pkl".format(dataset,RUNS),'wb')
        )


plot_speeds(speeds, dataset, show_legend)
plt.show()

