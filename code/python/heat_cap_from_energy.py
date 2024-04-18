"""Calculates the heat capacity from an E/N v kBT graph. The gradient is calculated using a least-square fit along small sections of points
"""

import numpy as np
from scipy import optimize

INPUT_FILE = "./energy-1000.csv"
OUTPUT_FILE = "./heat-cap-1000.csv"
AVG_POINTS = 20 #(Either side)

#Read in data
energyData = []

fileBuff = open(INPUT_FILE, 'r')
for line in fileBuff.read().split("\n"):
    line = line.split(",")

    if len(line) == 2:
        try:
            energyData.append((float(line[0]), float(line[1])))
        except ValueError:
            pass
fileBuff.close()

def square_distance(y_value, fit_value):
    return (y_value - fit_value)**2

def MinFunc(yVals, yFit):
    return np.sum(square_distance(yVals, yFit))

#Calculate gradients
gradData = []
for energyIndex in range(AVG_POINTS, len(energyData) - AVG_POINTS):

    xValues = []
    yValues = []
    gradientAvg = 0.0
    for sampleIndex in range(energyIndex - AVG_POINTS, energyIndex + AVG_POINTS):
        #gradientAvg += (energyData[sampleIndex][1] - energyData[sampleIndex - 1][1]) / (energyData[sampleIndex][0] - energyData[sampleIndex - 1][0])
        xValues.append(energyData[sampleIndex][0])
        yValues.append(energyData[sampleIndex][1])

    xValues = np.array(xValues)
    yValues = np.array(yValues)

    gradient = optimize.fmin(lambda grad : MinFunc(yValues, grad * (xValues - xValues[0]) + yValues[0]), x0=1.0)

    #gradientAvg /= AVG_POINTS * 2.0 - 1.0
    gradData.append((energyData[energyIndex][0], gradient[0]))

fileBuff = open(OUTPUT_FILE, 'w')
outData = "temp,heat-cap\n"
for data in gradData:
    outData += str(data[0]) + "," + str(data[1]) + "\n"

fileBuff.write(outData)
fileBuff.close()