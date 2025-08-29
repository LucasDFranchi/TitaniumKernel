import json
import time
import paho.mqtt.client as mqtt
import pytest


class MqttTestClient:
    def __init__(self, broker="broker.hivemq.com", port=1883, client_id="pytest-client"):
        self.client = mqtt.Client(client_id=client_id)
        self.client.connect(broker, port, 60)
        self.client.loop_start()
        self._messages = {}
        self.client.on_message = self._on_message

    def _on_message(self, client, userdata, msg):
        payload = json.loads(msg.payload.decode())
        topic = msg.topic
        if topic not in self._messages:
            self._messages[topic] = []
        self._messages[topic].append(payload)

    def subscribe(self, topic):
        self.client.subscribe(topic)
        time.sleep(0.2)  # ensure subscription is active

    def publish(self, topic, payload):
        self.client.publish(topic, json.dumps(payload))

    def wait_for_message(self, topic, timeout=5):
        start = time.time()
        while time.time() - start < timeout:
            if topic in self._messages and self._messages[topic]:
                return self._messages[topic].pop(0)
            time.sleep(0.1)
        pytest.fail(f"Timeout waiting for message on topic '{topic}'")

    def stop(self):
        self.client.loop_stop()
        self.client.disconnect()
