import subprocess
import matplotlib as mpl
from matplotlib import pyplot as plt
import numpy as np
import cPickle

phindex = int(raw_input("Enter phindex: "))
phi = 2**-phindex

show_legend = True if raw_input("Show legend? (y/n)")=="y" else False

RUNS = 8

# do latex figures
text_width = 7.1413
golden_mean = (np.sqrt(5)-1.0)/2.0      # Aesthetic ratio
fig_width = text_width/3                # width in inches
fig_height = fig_width*golden_mean      # height in inches
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

zs = np.arange(0.1,2.1,0.1)
fn = "../src/Release/hh-zipf.exe"

try:
    speeds = cPickle.load(open(
        "../results/Skew {} Runs {} Phindex Gamma 4 Single Run.pkl".format(RUNS, phindex),
        'rb'))
except:
    speeds = []
    for run in xrange(RUNS):
        results = {}
        first_iteration = True
        for z in zs:
            command = [fn,"-z", str(z), "-phi", str(phi), "-r", "1"]
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
        speed = results["Updates/ms"]
        speeds.append(speed)

    cPickle.dump(
        speeds,
        open("../results/Skew {} Runs {} Phindex Gamma 4 Single Run.pkl".format(RUNS, phindex),'wb')
        )


try:
    speeds_gamma_1 = cPickle.load(open(
        "../results/Skew {} Runs {} Phindex Gamma 1 Single Run.pkl".format(RUNS, phindex),
        'rb'))
except:
    speeds_gamma_1 = []
    for run in xrange(RUNS):
        results = {}
        first_iteration = True
        for z in zs:
            command = [fn,"-z", str(z), "-phi", str(phi), "-r", "1",
                       "-gamma","1"]
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
        speed = results["Updates/ms"]
        speeds_gamma_1.append(speed)

    cPickle.dump(
        speeds_gamma_1,
        open("../results/Skew {} Runs {} Phindex Gamma 1 Single Run.pkl".format(RUNS,
                                                                     phindex),
             'wb')
        )


average_speeds = {}
for algorithm in speeds[0].keys():
	average_speeds[algorithm] = [
            sum([speed[algorithm][i] for speed in speeds])/RUNS
            for i in xrange(len(speeds[0][algorithm]))]


average_speeds_gamma_1 = {}
for algorithm in speeds_gamma_1[0].keys():
	average_speeds_gamma_1[algorithm] = [
            sum([speed[algorithm][i] for speed in speeds_gamma_1])/RUNS
            for i in xrange(len(speeds_gamma_1[0][algorithm]))]


plt.plot(zs, average_speeds["ALS"], "-D", label="IM-SUM $\gamma=4$")
plt.plot(zs, average_speeds_gamma_1["ALS"], "--D", color = 'b', label="IM-SUM $\gamma=1$")
plt.plot(zs, average_speeds["LS"],"-d",label="DIM-SUM $\gamma=4$")
plt.plot(zs, average_speeds_gamma_1["LS"],"--d", color = 'g', label="DIM-SUM $\gamma=1$")
plt.plot(zs, average_speeds["SSH"],"-o",label="SSH")
plt.plot(zs, average_speeds["CM"],"-+",label="CM")
plt.plot(zs, average_speeds["CMH"],"-^",label="CMH")
plt.plot(zs, average_speeds["CCFC"],"-v",label="AGT")


plt.xlabel("Skew")
plt.ylabel("Updates/ms")
if show_legend:
    plt.legend(loc="center")
plt.tight_layout()
fig = plt.gcf()
fig.canvas.set_window_title("Skew {} Runs {} Phindex Single Run".format(RUNS,
                                                                     phindex))

plt.show()
