import array
import numpy.random as rand
import numpy as np

GRID_SIZE = 10
INTERACTION_STRENGTH = 0.1 #Factor multiplied by spins in hamiltonian
TEMPERATURE_COEFF = 0.5 #Value of k_B * T

#2D for now
class SpinGrid():
    def __init__(self, sizeX, sizeY):
        self._sizeX = sizeX
        self._sizeY = sizeY

        #Build grid of 0s (to be populated with -1 or +1 for spins)
        self._grid = array.array("i", [0 for i in range(sizeX * sizeY)])

    def SetSpin(self, xPos, yPos, value):
        if value == 1 or value == -1:
            self._grid[xPos * self._sizeY + yPos] = value

    def GetSpin(self, xPos, yPos):
        return self._grid[xPos * self._sizeY + yPos]

    def DisplayGrid(self): #Replace with graphical representation
        for yPos in range(self._sizeY):
            outStr = ""
            for xPos in range(self._sizeX):
                if self.GetSpin(xPos, yPos) == 1:
                    outStr += "^"
                elif self.GetSpin(xPos, yPos) == -1:
                    outStr += "v"
                else:
                    outStr += " "
            print(outStr + "\n")

    def GetNearestNeighbours(self, xPos, yPos):
        neighbours = []
        if xPos > 0:
            neighbours.append((xPos - 1, yPos))
        if xPos < self._sizeX - 1:
            neighbours.append((xPos + 1, yPos))
        if yPos > 0:
            neighbours.append((xPos, yPos - 1))
        if yPos < self._sizeY - 1:
            neighbours.append((xPos, yPos + 1))

        return neighbours

    def CalculateEnergy(self):
        totalEnergy = 0.0
        for xPos in range(self._sizeX):
            for yPos in range(self._sizeY):
                #Iterate over nearest neighbours
                for neighbour in self.GetNearestNeighbours(xPos, yPos):
                    totalEnergy += self.GetSpin(xPos, yPos) * self.GetSpin(neighbour[0], neighbour[1])

        return totalEnergy * INTERACTION_STRENGTH

    def Iterate(self):
        #Choose a random spin
        xPos = rand.randint(0, self._sizeX)
        yPos = rand.randint(0, self._sizeY)

        #Calculate energy change if this spin flips
        newSpin = self.GetSpin(xPos, yPos) * -1
        energyChange = 0.0
        for neighbour in self.GetNearestNeighbours(xPos, yPos):
            energyChange += newSpin * self.GetSpin(neighbour[0], neighbour[1])
        energyChange *= INTERACTION_STRENGTH

        #Conditions to flip spin
        if energyChange <= 0 or rand.random() <= np.exp(-energyChange / TEMPERATURE_COEFF):
            self.SetSpin(xPos, yPos, newSpin)