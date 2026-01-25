import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# -----------------------------
# CONFIG
# -----------------------------
NOISE_STD = 0.01      # adjust noise strength: 0.05 = mild, 0.1 = normal
TARGET_DAYS = 30     # extend to 30 days
WINDOW_DAYS = 3      # original window length
# -----------------------------

# Load CSV (no header, first two columns only)
df = pd.read_csv(
    "sensor_5.csv",
    header=None,
    usecols=[0, 1],
    names=["timestamp", "value"],
    sep=r"[\s,;]+"
)

# Convert timestamps
df["datetime"] = pd.to_datetime(df["timestamp"], unit="s")
df = df.sort_values("datetime")  # ensure sorted


# Identify time window
start_time = df["datetime"].iloc[0]
end_time = start_time + pd.Timedelta(days=WINDOW_DAYS)

pattern_df = df[df["datetime"] < end_time].copy()
pattern_duration = pattern_df["timestamp"].iloc[-1] - pattern_df["timestamp"].iloc[0]

print(f"Pattern duration (s): {pattern_duration}")

# Number of repeats needed
total_seconds = TARGET_DAYS * 24 * 3600
repeats = int(np.ceil(total_seconds / pattern_duration))

# Build extended data
extended_list = []

for i in range(repeats):
    shift = i * pattern_duration

    temp = pattern_df.copy()
    temp["timestamp"] = temp["timestamp"] + shift
    temp["value"] = (temp["value"] + np.random.normal(0, NOISE_STD, size=len(temp))).rolling(window=100, center=True).mean()
    extended_list.append(temp)

extended_df = pd.concat(extended_list)

# Trim to exact 30 days
final_end_time = df["timestamp"].iloc[0] + total_seconds
extended_df = extended_df[extended_df["timestamp"] <= final_end_time]

# Convert timestamp again
extended_df["datetime"] = pd.to_datetime(extended_df["timestamp"], unit="s")

# Save output
extended_df[["timestamp", "value"]].to_csv("extended_30_days.csv", index=False)

print("Saved file: extended_30_days.csv")

# -----------------------------
# Plot
# -----------------------------

start_ts = extended_df["timestamp"].iloc[0]
timestamps = extended_df["timestamp"]

time_days = (timestamps - start_ts) / (24 * 3600)

plt.figure(figsize=(14, 5))
# plt.plot(df["datetime"], df["value"], label="Original 3 days", linewidth=2)
plt.plot(time_days, extended_df["value"], label="Sensor saudável (referência)", linewidth=1)
aging_factor = 0.0000005
start_after = 25000
# aging_factor = 0.00002

aged_values = [
    x if i < start_after else x + aging_factor * (i - start_after)
    for i, x in enumerate(extended_df["value"])
]

plt.plot(time_days, aged_values, label= "Sensor com envelhecimento", linewidth=1)



plt.xlabel("Linha do Tempo de Leitura (Dias)")
plt.ylabel("Temperatura (°C)")
plt.title("Efeito do Envelhecimento: Comparação entre Sensor Normal e Sensor Degradado")

plt.legend()
plt.grid(True)
plt.tight_layout()
# plt.ylim(-17, -18)

plt.show()
