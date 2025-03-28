import spin_grid
import array
import numpy.random as rand
import numpy as np

#Parameters
GRID_SIZE = 100
B_FIELD = 0.0
INTERACTION_STRENGTH = 1.0

TEMPERATURE_RANGE = (0.1, 3.0)
TEMPERATURE_COUNT = 50

MAX_ITERATIONS = 10000000 #Maximum iterations to run no matter what
SAMPLE_ITERATIONS = 100000 #The number of iterations to average the change over
TOLERANCE = 1e-3 #A tolerance value for considering a change large enough to mean the system hasn't reached equilibrium

OUTPUT_FILE_ENERGY = "./csv/energy-temp.csv"
OUTPUT_FILE_HEAT_CAP = "./csv/heatcap-temp.csv"

INITIAL_GRID = None

#Start with the same random grid each time
def InitRandomSpins(grid):
    global INITIAL_GRID

    setupValues = False
    if INITIAL_GRID == None:
        INITIAL_GRID = array.array("i", [0 for i in range(GRID_SIZE**2)])
        setupValues = True

    #Initial random spin arrangement 
    for i in range(grid._sizeX):
        for j in range(grid._sizeY):
            if setupValues:
                randNum = rand.randint(1, 3)
                
                if randNum == 1:
                    INITIAL_GRID[i * grid._sizeY + j] = 1
                else:
                    INITIAL_GRID[i * grid._sizeY + j] = -1
            
            grid.SetSpin(i, j, INITIAL_GRID[i * grid._sizeY + j])

def FindEquilibriumEnergy(temp):

    #Set up new grid
    grid = spin_grid.SpinGrid(GRID_SIZE, GRID_SIZE, B_FIELD, INTERACTION_STRENGTH)
    grid.SetTemperature(temp)
    InitRandomSpins(grid)

    #Last term accounts for edge cases
    minEnergy = -GRID_SIZE * GRID_SIZE * 4 * INTERACTION_STRENGTH - B_FIELD * GRID_SIZE * GRID_SIZE + GRID_SIZE * 4 * INTERACTION_STRENGTH
    energyTolerance = np.abs(minEnergy * TOLERANCE)

    energyValue = 0.0

    cycles = int(MAX_ITERATIONS / SAMPLE_ITERATIONS)
    for i in range(cycles):
        grid.Iterate(SAMPLE_ITERATIONS)

        #Calculate change
        if np.abs(grid._lastTotalEnergy - energyValue) < energyTolerance:
            energyValue = grid._lastTotalEnergy
            break
        else:
            energyValue = grid._lastTotalEnergy

    return energyValue

outputValues = []

tempInterval = (TEMPERATURE_RANGE[1] - TEMPERATURE_RANGE[0]) / (TEMPERATURE_COUNT - 1)
for tempNum in range(TEMPERATURE_COUNT):
    #Calculate temperature to test
    currentTemp = TEMPERATURE_RANGE[0] + tempInterval * tempNum

    outputValues.append((currentTemp, FindEquilibriumEnergy(currentTemp)))

    print(f"kBT = {currentTemp} J calculated")

file_buff = open(OUTPUT_FILE_ENERGY, 'w')
outputData = "temp,energy\n"
for values in outputValues:
    outputData += f"{values[0]},{values[1]}\n"
file_buff.write(outputData)
file_buff.close()

#Generate C_V/N
gradValues = []
for valueIndex in range(len(outputValues)):
    #Calculate gradient
    gradient = 0.0
    count = 0
    if valueIndex > 0:
        gradient += (outputValues[valueIndex][1] - outputValues[valueIndex - 1][1]) / (outputValues[valueIndex][0] - outputValues[valueIndex - 1][0])
        count += 1
    if valueIndex < len(outputValues) - 1:
        gradient += (outputValues[valueIndex + 1][1] - outputValues[valueIndex][1]) / (outputValues[valueIndex + 1][0] - outputValues[valueIndex][0])
        count += 1

    gradient /= count

    gradValues.append((outputValues[valueIndex][0] , gradient / (GRID_SIZE**2)))

file_buff = open(OUTPUT_FILE_HEAT_CAP, 'w')
outputData = "temp,heatcap\n"
for values in gradValues:
    outputData += f"{values[0]},{values[1]}\n"
file_buff.write(outputData)
file_buff.close()