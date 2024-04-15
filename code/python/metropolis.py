import numpy as np
import matplotlib.pyplot as plt

BOLTZMANN = 1.38064852e-23 # Boltzmann constant (J/K)

temperature = 100 # K
beta = 1 / (BOLTZMANN * temperature) # 1 / J

J = 1 # interaction energy (J)
H = 0 # external magnetic field (J)
size = 100 # lattice size
steps = 10000 # number of steps

plt.ion()
pauseTime = 0.01 # seconds

class Lattice:
    # initialize the lattice with random spins
    def __init__(self, size, J, H):
        self.size = size
        self.J = J
        self.H = H
        self.lattice = np.random.choice([-1, 1], size=(size, size))
        self.energy = []
        self.energy.append(-J * np.sum(
            self.lattice * (
                np.roll(self.lattice, 1, axis=0) +
                np.roll(self.lattice, 1, axis=1)
            )
        ) - H * np.sum(self.lattice))
        self.magnetization = []
        self.magnetization.append(np.sum(self.lattice))
        self.fig, self.axs = plt.subplots(1, 3, figsize=(14, 4))
        plt.subplots_adjust(wspace=0.5)

    # plot the lattice, energy, and magnetization
    def plot(self):
        step = len(self.energy)
        if step % 100 != 0: # plot every 100 steps, it's too slow to plot every step
            return
        self.axs[0].imshow(self.lattice, cmap='gray', vmin=-1, vmax=1)
        self.axs[0].set_xticks([])
        self.axs[0].set_yticks([])
        self.axs[1].plot(range(step), self.energy, color='black', lw=0.5)
        self.axs[1].set_xlabel('steps')
        self.axs[1].set_ylabel('energy')
        self.axs[2].plot(range(step), self.magnetization, color='black', lw=0.5)
        self.axs[2].set_xlabel('steps')
        self.axs[2].set_ylabel('magnetization')
        plt.pause(pauseTime)

    # try to flip a spin and calculate the change in energy and magnetization
    def flip(self, i, j):
        dE = 2 * self.J * self.lattice[i, j] * (
            self.lattice[(i - 1) % self.size, j] +
            self.lattice[(i + 1) % self.size, j] +
            self.lattice[i, (j - 1) % self.size] +
            self.lattice[i, (j + 1) % self.size] # periodic boundary conditions
        ) + 2 * self.H * self.lattice[i, j]
        if dE <= 0 or np.random.rand() < np.exp(-beta * dE):
            self.lattice[i, j] *= -1
            return dE, 2 * self.lattice[i, j]
        return 0, 0

    # run the Metropolis algorithm
    def metropolis(self, steps):
        for _ in range(steps):
            i, j = np.random.randint(self.size, size=2)
            dE, dM = self.flip(i, j)
            self.energy.append(self.energy[-1] + dE)
            self.magnetization.append(self.magnetization[-1] + dM)
            self.plot()
    

lattice = Lattice(size, J, H)
lattice.metropolis(steps)
print(lattice.energy[-1], lattice.magnetization[-1])

plt.ioff()
plt.show()
