import json
import paho.mqtt.client as mqtt

# MQTT broker details
BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC = "iocloud/response/1C69209DB778/#"
OUTPUT_FILE = "responses.jsonl"  # each line will contain one JSON

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Connected to MQTT broker")
        client.subscribe(TOPIC)
        print(f"📡 Subscribed to topic: {TOPIC}")
    else:
        print(f"❌ Connection failed with code {rc}")

def on_message(client, userdata, msg):
    payload = msg.payload.decode("utf-8")
    print(f"📩 Received on {msg.topic}: {payload}")
    try:
        data = json.loads(payload)
    except json.JSONDecodeError:
        print("⚠️ Not valid JSON, skipping...")
        return

    # Append JSON to file
    with open(OUTPUT_FILE, "a", encoding="utf-8") as f:
        json.dump(data, f)
        f.write("\n")

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"🔗 Connecting to {BROKER}:{PORT} ...")
    client.connect(BROKER, PORT, keepalive=60)

    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n🛑 Disconnected by user")
        client.disconnect()

if __name__ == "__main__":
    main()
