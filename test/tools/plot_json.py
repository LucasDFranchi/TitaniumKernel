import json
import matplotlib.pyplot as plt

# Path to your file
filename = "responses.jsonl"

# Initialize list for timestamps and sensor data
timestamps = []
sensor_values = [[] for _ in range(20)]  # first 20 sensors

# Read the file line by line
with open(filename, "r") as f:
    for line in f:
        data = json.loads(line.strip())
        timestamps.append(data["timestamp"])
        for i in range(20):
            sensor_values[i].append(data["sensors"][i]["value"])

# Plot all 20 sensors
plt.figure(figsize=(12, 6))
for i in range(20):
    plt.plot(timestamps, sensor_values[i], label=f"Sensor {i+1}")

plt.xlabel("Timestamp")
plt.ylabel("Value")
plt.title("First 20 Sensor Values Over Time")
plt.legend(ncol=2, fontsize="small")
plt.grid(True)
plt.tight_layout()
plt.show()
