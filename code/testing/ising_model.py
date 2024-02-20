import array, threading, time, sys
import numpy.random as rand
import numpy as np
import pygame

GRID_SIZE = 100
INTERACTION_STRENGTH = 1.0 #Factor multiplied by spins in hamiltonian
BETA = 1000 #Value of 1 / k_B * T
WINDOW_SIZE = (800,800)
MATRIX_BORDER = 100
ITERATION_COUNT = 10000

#2D for now
class SpinGrid():
    def __init__(self, sizeX, sizeY):
        self._sizeX = sizeX
        self._sizeY = sizeY
        self._thd = None
        self._threadFinished = True
        self._drawFinished = False

        #Build grid of 0s (to be populated with -1 or +1 for spins)
        self._grid = array.array("i", [0 for i in range(sizeX * sizeY)])

    def SetSpin(self, xPos, yPos, value):
        if value == 1 or value == -1:
            self._grid[xPos * self._sizeY + yPos] = value

    def GetSpin(self, xPos, yPos):
        return self._grid[xPos * self._sizeY + yPos]

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
                    totalEnergy += -self.GetSpin(xPos, yPos) * self.GetSpin(neighbour[0], neighbour[1])

        return totalEnergy * INTERACTION_STRENGTH

    def Iterate(self, repeats):
        
        self._threadFinished = False
        for i in range(repeats):
            #Choose a random spin
            xPos = rand.randint(0, self._sizeX)
            yPos = rand.randint(0, self._sizeY)

            #Calculate energy change if this spin flips
            newSpin = self.GetSpin(xPos, yPos) * -1
            energyChange = 0.0
            for neighbour in self.GetNearestNeighbours(xPos, yPos):
                energyChange += -2 * newSpin * self.GetSpin(neighbour[0], neighbour[1])
            energyChange *= INTERACTION_STRENGTH

            #Conditions to flip spin
            if energyChange <= 0 or rand.random() <= np.exp(-energyChange * BETA):
                self.SetSpin(xPos, yPos, newSpin)

        self._threadFinished = True

    def StartIterateThread(self, repeats):
        """Starts the iteration thread and doesn't start the next job until the results have been drawn.
        Parameters:
            repeats : How many iterations to perform before drawing to the screen.
        """

        if self._threadFinished and self._drawFinished:
            self._thd = threading.Thread(target=self.Iterate, args=(repeats,))
            self._thd.start()

            self._drawFinished = False

    def Draw(self):
        #Only draws if thread has finished
        if not self._threadFinished:
            return

        startTime = time.time()

        window.fill([150,150,150])

        #Range for matrix
        xRange = (MATRIX_BORDER, WINDOW_SIZE[0] - MATRIX_BORDER)
        xSize = (xRange[1] - xRange[0]) / self._sizeX

        yRange = (MATRIX_BORDER, WINDOW_SIZE[1] - MATRIX_BORDER)
        ySize = (xRange[1] - xRange[0]) / self._sizeX

        for xIndex in range(self._sizeX):
            xPos = xRange[0] + xIndex * xSize
            for yIndex in range(self._sizeY):
                yPos = yRange[0] + yIndex * ySize

                if self.GetSpin(xIndex, yIndex) == 1:
                    col = [0,0,0]
                else:
                    col = [255,255,255]

                pygame.draw.rect(window, col, pygame.Rect(xPos, yPos, xSize, ySize))

        self._drawFinished = True

        print("Time: " + str(time.time() - startTime))

def InitSpins(grid):
    #Initial random spin arrangement 
    for i in range(grid._sizeX):
        for j in range(grid._sizeY):
            randNum = rand.randint(1, 3)
            if randNum == 1:
                grid.SetSpin(i, j, 1)
            else:
                grid.SetSpin(i, j, -1)

def Update():
    grid.StartIterateThread(ITERATION_COUNT)
    grid.Draw()

grid = SpinGrid(GRID_SIZE, GRID_SIZE)
InitSpins(grid)

window = pygame.display.set_mode(WINDOW_SIZE, pygame.HWSURFACE | pygame.DOUBLEBUF)
clock = pygame.time.Clock()

while True:
    clock.tick(30)

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit(0)

    Update()
    pygame.display.flip()