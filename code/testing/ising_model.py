import array, threading, time, sys
import numpy.random as rand
import numpy as np
import pygame

####Constants
BOLTZMANN = 1.38e-23

####Editable
GRID_SIZE = 200
INTERACTION_STRENGTH = 1.0 #Factor multiplied by spins in hamiltonian
TEMPERATURE = 273 #Temperature value
WINDOW_SIZE = (1600,800)
ITERATION_COUNT = 10000

####
BETA = 1.0 / (TEMPERATURE * BOLTZMANN)

#2D for now
class SpinGrid():
    def __init__(self, sizeX, sizeY):
        self._sizeX = sizeX
        self._sizeY = sizeY
        self._thd = None
        self._threadFinished = True
        self._drawFinished = False

        self._graphPoints = []
        self._iterationNum = 0

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

        randomX = rand.randint(0, self._sizeX, repeats)
        randomY = rand.randint(0, self._sizeY, repeats)
        randomFloats = rand.random(repeats)

        for i in range(repeats):
            #Choose a random spin
            xPos = randomX[i]
            yPos = randomY[i]

            #Calculate energy change if this spin flips
            newSpin = self.GetSpin(xPos, yPos) * -1
            energyChange = 0.0
            for neighbour in self.GetNearestNeighbours(xPos, yPos):
                energyChange += -2 * newSpin * self.GetSpin(neighbour[0], neighbour[1])
            energyChange *= INTERACTION_STRENGTH

            #Conditions to flip spin
            if energyChange <= 0 or randomFloats[i] <= np.exp(-energyChange * BETA):
                self.SetSpin(xPos, yPos, newSpin)

        self._threadFinished = True
        self._iterationNum += repeats

    def StartIterateThread(self, repeats):
        """Starts the iteration thread and doesn't start the next job until the results have been drawn.
        Parameters:
            repeats : How many iterations to perform before drawing to the screen.
        """

        if self._threadFinished and self._drawFinished:
            self._thd = threading.Thread(target=self.Iterate, args=(repeats,))
            self._thd.start()

            self._drawFinished = False

    def BuildGraph(self, currentWindowSize, matrixSize):

        ratio = 1.5 #x to y ratio
        graphSize = currentWindowSize[0] / 2 - 100 #x size
        yMidpoint = 50 + matrixSize / 2

        pygame.draw.rect(window, [255, 255, 255], pygame.Rect(currentWindowSize[0]/2 + 50, yMidpoint - graphSize / (2 * ratio), graphSize, graphSize / ratio))
    
    def Draw(self):
        #Only draws if thread has finished
        if not self._threadFinished:
            return

        currentWindowSize = window.get_size()

        window.fill([200,200,200])

        #Text
        text_surface = main_font.render(f'Temperature: {TEMPERATURE} K    Iteration: {self._iterationNum}', True, (0, 0, 0))
        window.blit(text_surface, (50, 10))
    
        #Range for matrix
        matrixSize = np.min([currentWindowSize[0] / 2, currentWindowSize[1]]) - 100
        pixelSize = (np.floor(matrixSize / self._sizeX), np.floor(matrixSize / self._sizeY))

        for xIndex in range(self._sizeX):
            xPos = 50 + pixelSize[0] * xIndex
            for yIndex in range(self._sizeY):
                yPos = 50 + pixelSize[1] * yIndex

                if self.GetSpin(xIndex, yIndex) == 1:
                    col = [0,0,0]
                else:
                    col = [255,255,255]

                pygame.draw.rect(window, col, pygame.Rect(int(xPos), int(yPos), pixelSize[0], pixelSize[1]))

        self._drawFinished = True

        self.BuildGraph(currentWindowSize, matrixSize)

def InitSpins(grid):
    #Initial random spin arrangement 
    for i in range(grid._sizeX):
        for j in range(grid._sizeY):
            randNum = rand.randint(1, 3)
            if randNum == 1:
                grid.SetSpin(i, j, 1)
            else:
                grid.SetSpin(i, j, -1)

def InitPygame():

    pygame.init()
    window = pygame.display.set_mode(WINDOW_SIZE, pygame.HWSURFACE | pygame.DOUBLEBUF | pygame.RESIZABLE)
    pygame.display.set_caption("Ising Model Simulation")

    return window

def Update():
    grid.StartIterateThread(ITERATION_COUNT)
    grid.Draw()

grid = SpinGrid(GRID_SIZE, GRID_SIZE)
InitSpins(grid)

window = InitPygame()
clock = pygame.time.Clock()
main_font = pygame.font.Font("./font/cmu-serif-roman.ttf", size=20)

while True:
    clock.tick(30)

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit(0)

    Update()
    pygame.display.flip()