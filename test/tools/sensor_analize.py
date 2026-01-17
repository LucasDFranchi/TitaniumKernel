# sensor5_drift_analysis.py
import pandas as pd
import numpy as np
from datetime import datetime
import matplotlib.pyplot as plt
import os

csv_path = "data.csv"   # change if needed
# csv_path = "failing_sensor.csv"

# Load CSV robustly (skip malformed rows)
df = pd.read_csv(csv_path, header=None, sep=",", engine="python", on_bad_lines="skip")
df = df.reset_index(drop=True)

# Parse timestamp -> datetime, seconds and hours from start
df['datetime'] = df[0].apply(lambda ts: datetime.fromtimestamp(float(ts)))
df['time_s'] = df['datetime'].astype('int64') // 10**9
df['time_hours'] = (df['time_s'] - df['time_s'].iloc[0]) / 3600.0

# Sensor 0 is column 1
df['sensor5'] = pd.to_numeric(df[3*5+1], errors='coerce')

# Drop rows without sensor5
orig_len = len(df)
df = df.dropna(subset=['sensor5']).reset_index(drop=True)
dropped = orig_len - len(df)

# Basic stats
stats = df['sensor5'].describe()
print("Basic stats for sensor5:\n", stats)

# Sampling interval (median)
if len(df) >= 2:
    deltas = np.diff(df['time_s'].values)
    median_dt = float(np.median(deltas))
else:
    median_dt = np.nan
print("Median sampling interval (s):", median_dt)

# Rolling window sizes in samples for 1h, 6h, 24h
def hours_to_window(h):
    if np.isnan(median_dt) or median_dt <= 0:
        return 1
    return max(1, int(round((h * 3600) / median_dt)))

w1 = hours_to_window(1)
w6 = hours_to_window(6)
w24 = hours_to_window(24)
print("Rolling windows (samples) 1h,6h,24h:", w1, w6, w24)

# Rolling mean & std (1h)
df['rolling_mean_1h'] = df['sensor5'].rolling(window=w1, min_periods=1).mean()
df['rolling_std_1h'] = df['sensor5'].rolling(window=w1, min_periods=1).std().fillna(0)

# Rolling slope (drift units/hour) using linear fit on 1h window
def rolling_slope(x_time, x_val):
    if len(x_time) < 2:
        return 0.0
    a, b = np.polyfit(x_time, x_val, 1)  # a = units/hour
    return a

times = df['time_hours'].values
vals  = df['sensor5'].values
slopes = np.full(len(df), np.nan)
half = max(1, w1 // 2)
for i in range(len(df)):
    start = max(0, i - half)
    end = min(len(df), i + half + 1)
    wt = times[start:end]
    wv = vals[start:end]
    if len(wt) >= 2:
        slopes[i] = rolling_slope(wt, wv)
df['drift_per_hour_1h'] = slopes

# Global drift (linear regression over entire dataset)
if len(df) >= 2:
    global_a, global_b = np.polyfit(df['time_hours'].values, df['sensor5'].values, 1)
else:
    global_a, global_b = (np.nan, np.nan)
print(f"Global drift: {global_a:.6f} units/hour (intercept {global_b:.3f})")

# Sample-to-sample differences and spike detection
df['diff'] = df['sensor5'].diff().fillna(0)
diff_std = df['diff'].std()
abs_threshold = max(0.1, 6 * diff_std)   # tunable
spike_mask = df['diff'].abs() > abs_threshold
spikes = df[spike_mask].copy()
print("Spikes detected:", spikes.shape[0])

# Drift alerts where rolling drift exceeds threshold
drift_threshold = max(0.01, abs(global_a) * 0.2)  # 20% of global slope or min 0.01 units/h
df['drift_alert'] = df['drift_per_hour_1h'].abs() > drift_threshold
num_alerts = df['drift_alert'].sum()
print("Drift-alert points:", int(num_alerts), "threshold (units/h):", drift_threshold)

# SNR estimate
noise_est = df['rolling_std_1h'].mean()
signal_est = df['rolling_mean_1h'].abs().mean() if df['rolling_mean_1h'].abs().mean() != 0 else 1.0
snr = signal_est / (noise_est if noise_est != 0 else 1.0)
print("Estimated SNR:", snr)

# Save detected events and summary
events = pd.DataFrame({
    'datetime': spikes['datetime'],
    'timestamp': spikes['time_s'],
    'sensor5': spikes['sensor5'],
    'diff': spikes['diff']
}).reset_index(drop=True)
events_csv = "sensor5_detected_events.csv"
events.to_csv(events_csv, index=False)

summary = {
    'rows_total': int(orig_len),
    'rows_after_drop': int(len(df)),
    'rows_dropped': int(dropped),
    'median_sampling_s': float(median_dt),
    'global_drift_units_per_hour': float(global_a),
    'global_intercept': float(global_b),
    'drift_threshold_units_per_hour': float(drift_threshold),
    'spike_threshold': float(abs_threshold),
    'num_spikes_detected': int(spikes.shape[0]),
    'snr_estimate': float(snr)
}
summary_df = pd.DataFrame([summary])
summary_csv = "sensor5_summary.csv"
summary_df.to_csv(summary_csv, index=False)

print("Saved:", events_csv, summary_csv)

# Plots (each plot on its own figure)
plt.figure(figsize=(12,4))
plt.plot(df['datetime'], df['sensor5'], linewidth=1.2, label="sensor5")
plt.plot(df['datetime'], df['rolling_mean_1h'], linewidth=1.5, label="rolling_mean_1h")
plt.scatter(spikes['datetime'], spikes['sensor5'], marker='x', s=40, label="spikes")
plt.title("sensor5 values with 1h rolling mean and spikes")
plt.xlabel("Time"); plt.ylabel("sensor5"); plt.legend(); plt.grid(True); plt.tight_layout()
plt.show()

plt.figure(figsize=(12,4))
plt.plot(df['datetime'], df['drift_per_hour_1h'], linewidth=1.2, label="drift_per_hour_1h")
plt.axhline(global_a, linestyle='--', label=f"global drift {global_a:.6f} units/h")
plt.title("Rolling drift (units per hour)"); plt.xlabel("Time"); plt.ylabel("Drift (units/hour)")
plt.legend(); plt.grid(True); plt.tight_layout(); plt.show()

plt.figure(figsize=(10,4))
plt.plot(df['datetime'], df['diff'].abs(), linewidth=1.2, label="abs diff")
plt.axhline(abs_threshold, linestyle='--', label=f"spike threshold {abs_threshold:.4f}")
plt.title("Absolute sample-to-sample difference"); plt.xlabel("Time"); plt.ylabel("|diff|")
plt.legend(); plt.grid(True); plt.tight_layout(); plt.show()

plt.figure(figsize=(8,4))
plt.hist(df['sensor5'].dropna(), bins=80)
plt.title("Histogram of sensor5 values"); plt.xlabel("sensor5"); plt.ylabel("Count"); plt.tight_layout(); plt.show()

# Print short summary
print("\nSUMMARY:")
for k,v in summary.items():
    print(f" - {k}: {v}")
print("\nEvents saved to:", events_csv)
print("Summary saved to:", summary_csv)
