import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from datetime import timedelta

CSV_FILE = "data.csv"
NUM_DAYS_TARGET = 30
DRIFT_PER_DAY = 0.01
DEV_DAYS = 3
DEV_SCALE = 0.05

df = pd.read_csv(CSV_FILE, header=None)
df.rename(columns={0: "timestamp"}, inplace=True)
df["timestamp"] = pd.to_datetime(df["timestamp"], unit='s')

sensor_cols = df.columns[1:]
time_delta = df["timestamp"].diff().median()
samples_per_day = int(timedelta(days=1) / time_delta)

total_samples = NUM_DAYS_TARGET * samples_per_day
extra_samples = total_samples - len(df)

future_times = [df["timestamp"].iloc[-1] + (i+1)*time_delta
                for i in range(extra_samples)]
future_df = pd.DataFrame({"timestamp": future_times})

for col in sensor_cols:
    data = df[col].astype(float).values

    # Smooth original data
    smooth = pd.Series(data).rolling(50, min_periods=1).mean().values

    # Estimate slope from last 200 samples
    last_n = min(200, len(smooth))
    t = np.arange(last_n)
    coef = np.polyfit(t, smooth[-last_n:], 1)
    slope = coef[0]

    # If flat, reduce slope dramatically (avoids jumps)
    if abs(slope) < 1e-4:
        slope = 0

    # Extend using last known trend
    t_future = np.arange(1, extra_samples+1)
    base = smooth[-1] + slope * t_future

    # Add small positive drift
    drift = DRIFT_PER_DAY * (t_future / samples_per_day)

    # Deviation only last 3 days (smooth random walk)
    deviation = np.zeros(extra_samples)
    dev_len = DEV_DAYS * samples_per_day
    if dev_len > 0:
        noise = np.cumsum(np.random.normal(0, DEV_SCALE, dev_len))
        deviation[-dev_len:] = noise

    future_df[col] = base + drift + deviation

final = pd.concat([df, future_df], ignore_index=True)

# Example plot
sensor = df.columns[6]
plt.figure(figsize=(14, 6))
plt.plot(df["timestamp"], df[sensor], label="Original", linewidth=2)
plt.plot(future_df["timestamp"], future_df[sensor], label="Extended", linewidth=2)
plt.title(f"Sensor Drift – {sensor}")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()

final.to_csv("extended_clean_30_days.csv", index=False)
print("✔ Clean 30-day file saved: extended_clean_30_days.csv")
