import json
import pytest
import time
import paho.mqtt.client as mqtt
import threading


@pytest.fixture
def mqtt_client():
    client = mqtt.Client()
    client.connect("broker.hivemq.com", 1883, 60)
    client.loop_start()  # Must be started to process incoming messages
    yield client
    client.loop_stop()
    client.disconnect()


def wait_for_response(mqtt_client, topic, timeout=5):
    messages = []

    def on_message(client, userdata, msg):
        messages.append(json.loads(msg.payload.decode()))

    mqtt_client.subscribe(topic)
    mqtt_client.on_message = on_message
    time.sleep(0.2)  # Give time for subscription to register

    start = time.time()
    while time.time() - start < timeout:
        if messages:
            return messages[0]
        time.sleep(0.1)

    pytest.fail(f"Timeout waiting for message on topic '{topic}'")


def test_set_calibration_command(mqtt_client):
    topic_resp = "iocloud/response/1C69209DFC08/command"
    mqtt_client.subscribe(topic_resp)  # Subscribe before publish

    payload = {"command": 1, "params": {"sensor_id": 2, "gain": 1.23, "offset": 0.04}}

    mqtt_client.publish("iocloud/request/1C69209DFC08/command", json.dumps(payload))

    response = wait_for_response(mqtt_client, topic_resp, timeout=10)
    print(response)
    assert response["command_index"] == 1
    assert response["command_status"] == 0
    assert response.get("sensor_id") == 2


def test_bulk_calibration_commands_sequential(mqtt_client):
    topic_req = "iocloud/request/1C69209DFC08/command"
    topic_resp = "iocloud/response/1C69209DFC08/command"

    for i in range(1000):
        sensor_id = i % 10
        gain = round(1.0 + i * 0.001, 3)
        offset = round(0.01 + i * 0.0001, 4)

        payload = {
            "command": 1,
            "params": {
                "sensor_id": sensor_id,
                "gain": gain,
                "offset": offset
            }
        }

        mqtt_client.publish(topic_req, json.dumps(payload))

        response = wait_for_response(mqtt_client, topic_resp, timeout=5)

        print(f"[{i+1}/1000] Response: {response}")

        # Basic checks
        assert response["command_index"] == 1
        assert response["command_status"] == 0
        assert response.get("sensor_id") == sensor_id
