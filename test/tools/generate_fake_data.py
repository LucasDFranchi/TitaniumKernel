import numpy as np
import pandas as pd
from datetime import datetime, timedelta

# Parameters
N = 5000  # number of samples
base_value = 25.4
sampling_s = 6

# Time axis
t0 = datetime.now()
timestamps = [t0 + timedelta(seconds=i * sampling_s) for i in range(N)]

# Drift (non-linear: accelerates at the end)
drift = np.linspace(0, 4.0, N) ** 1.3 / 5  

# Noise increasing over time
noise = np.random.normal(0, np.linspace(0.7, 2.5, N))

# Occasional spikes
spikes = np.zeros(N)
spike_indices = np.random.choice(N, size=50, replace=False)
spikes[spike_indices] = np.random.choice([1, -1], size=50) * np.random.uniform(0.5, 1.5, size=50)

# Sudden offset jump at 70% of lifetime
jump = np.zeros(N)
jump_index = int(N * 0.7)
jump[jump_index:] = 1.5  # sudden +1.5 unit bias

# Final signal
sensor = base_value + drift + noise + spikes + jump

# Save
df = pd.DataFrame({
    "timestamp": [int(ts.timestamp()) for ts in timestamps],
    "sensor0": sensor
})

df.to_csv("failing_sensor.csv", index=False)
print("Generated failing_sensor.csv")
