import json
import pytest
import time
import paho.mqtt.client as mqtt
import threading

unit_test_list = ["°C", "°C", "°C", "°C", "°C", "°C", "°C", "°C", "°C", "°C","°C", "°C", "°C", "°C", "°C", "°C", "°C", "°C", "°C", "°C", "kPa", "kPa", "V", "A", "%"]

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

    gain = 1.00
    offset = 0.00
    payload = {"command": 1, "params": {"sensor_id": 2, "gain": gain, "offset": offset}}

    mqtt_client.publish("iocloud/request/1C69209DFC08/command", json.dumps(payload))
    
    response = wait_for_response(mqtt_client, topic_resp, timeout=25)
    
    assert response["command_index"] == 1
    assert response["command_status"] == 0
    assert response.get("sensor_id") == 2
    assert response.get("unit") == unit_test_list[response.get("sensor_id", 0)]
    assert response.get("gain") == gain
    assert response.get("offset") == offset
    
def test_set_calibration_check_all_units(mqtt_client):
    topic_resp = "iocloud/response/1C69209DFC08/command"
    mqtt_client.subscribe(topic_resp)  # Subscribe before publish

    for sensor_id in range(0, 25):
        gain = 1.00
        offset = 0.00
        payload = {"command": 1, "params": {"sensor_id": sensor_id, "gain": gain, "offset": offset}}

        mqtt_client.publish("iocloud/request/1C69209DFC08/command", json.dumps(payload))

        response = wait_for_response(mqtt_client, topic_resp, timeout=25)

        assert response.get("command_index") == 1
        assert response.get("command_status") == 0
        assert response.get("sensor_id") == sensor_id
        assert response.get("unit") == unit_test_list[response.get("sensor_id", 0)]
        assert response.get("gain") == gain
        assert response.get("offset") == offset
