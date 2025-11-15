import pandas as pd
import numpy as np
import numpy as np
import matplotlib.pyplot as plt
from sklearn.metrics import r2_score, mean_squared_error

df = pd.read_csv("resistance_data.csv")

measured = df["Measured Resistance (kΩ)"].astype(str).str.replace(",", ".").astype(float)
calculated = df["Calculated Resistance (kΩ)"].astype(str).str.replace(",", ".").astype(float)

# ==============================
# Input your data here
# ==============================
# Reference (true / measured resistance)
reference = measured

# Adjusted (calculated) values from your sensor
adjusted = calculated

# === Split into 5 regions ===
n_regions = 5
region_indices = np.array_split(np.arange(len(measured)), n_regions)

fits = []
predicted = []
plt.figure(figsize=(8, 5))
plt.scatter(measured, calculated, color='black', label='Data')

for i, idx in enumerate(region_indices):
    x = calculated[idx]
    y = measured[idx]

    coeffs = np.polyfit(x, y, 2)  # quadratic fit
    p = np.poly1d(coeffs)
    fits.append(coeffs)

    x_fit = np.linspace(x.min(), x.max(), 50)
    y_fit = p(x_fit)
    plt.plot(x_fit, y_fit, label=f'Region {i+1}')
    
    residuals = y - p(x)
    rmse = np.sqrt(np.mean(residuals**2))
    predicted.extend(p(adjusted[idx]))

    print(f'=== Region {i+1} ===')
    print(f'Range: {x.min():.3f} – {x.max():.3f} kΩ')
    print(f'Coefficients: a={coeffs[0]:.6e}, b={coeffs[1]:.6e}, c={coeffs[2]:.6e}')
    print(f'RMSE: {rmse:.4f} kΩ\n')
    
for i in range(len(reference)):
    print(f"Measured: {reference[i]:.3f} kΩ, Calculated: {adjusted[i]:.3f} kΩ, Fitted: {predicted[i]:.3f} kΩ")

plt.xlabel("Calculated (kΩ)")
plt.ylabel("Measured (kΩ)")
plt.title("5-Region Quadratic Calibration Fit")
plt.legend()
plt.grid(True)
plt.show()
