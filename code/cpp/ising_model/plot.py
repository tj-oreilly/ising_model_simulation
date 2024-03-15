import pandas as pd
import matplotlib.pyplot as plt

csv_files = ['energy.csv', 'heatCap.csv', 'magnetisation.csv']
for i, file in enumerate(csv_files):
    df = pd.read_csv(file)
    x = df.iloc[:, 0]
    y = df.iloc[:, 1]
    labels = df.columns.tolist()
    plt.subplot(3, 1, i+1)
    plt.plot(x, y, label=file)
    plt.xlabel(labels[0])
    plt.ylabel(labels[1])

plt.tight_layout()
plt.show()
