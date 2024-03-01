import spin_grid

#Parameters
GRID_SIZE = 100
B_FIELD = 0.0
INTERACTION_STRENGTH = 1.0
TEMPERATURE_RANGE = (0.1, 1000)

#Repeat for a range of temperatures to calculate the equilibrium energy
#Hopefully produce a plot of E against T and then one for C_V/N against T

def InitSpins():
    pass

grid = spin_grid.SpinGrid(GRID_SIZE, GRID_SIZE, B_FIELD, INTERACTION_STRENGTH)
grid.SetTemperature(0.0)
InitSpins(grid)