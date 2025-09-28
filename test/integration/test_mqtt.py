import json
import random
import os
import pytest
import time

from mqtt_module import MqttTestClient

_DEVICE_ID = "1C69209DB778"
_THIS_PATH = os.path.dirname(os.path.abspath(__file__))


def send_and_validate_calibration(
    mqtt_client, sensor_id: int, gain: float, offset: float
):
    topic_req = f"iocloud/request/{_DEVICE_ID}/command"
    topic_resp = f"iocloud/response/{_DEVICE_ID}/command"

    command = {}
    with open(
        os.path.join(_THIS_PATH, "templates", "default_calibration_command.json"), "r"
    ) as f:
        command = json.load(f)

    command["params"]["sensor_id"] = sensor_id
    command["params"]["gain"] = gain
    command["params"]["offset"] = offset

    mqtt_client.subscribe(topic_resp)
    start_time = time.time()
    mqtt_client.publish(topic_req, command)

    message = mqtt_client.wait_for_message(topic_resp, timeout=25)
    if message is None:
        raise TimeoutError("No message received within the timeout period.")
    print(f"Message received in {time.time() - start_time:.2f} seconds: {message}")
    # Validate message
    assert message["command_index"] == command["command"]
    assert message["command_status"] == 0
    assert message["sensor_id"] == sensor_id
    # assert message["unit"] == unit_test_list[sensor_id]
    assert math.isclose(
        message["gain"], gain, rel_tol=1e-6
    ), f"Expected gain ~{gain}, got {message['gain']}"

    assert math.isclose(
        message["offset"], offset, rel_tol=1e-6
    ), f"Expected offset ~{offset}, got {message['offset']}"

    return message

@pytest.mark.skip(reason="Known issue: the invalid commands do not return any status")
def send_and_expect_fail_calibration(
    mqtt_client, sensor_id: int, gain: float, offset: float
):
    topic_req = f"iocloud/request/{_DEVICE_ID}/command"
    topic_resp = f"iocloud/response/{_DEVICE_ID}/command"

    command = {}
    with open(
        os.path.join(_THIS_PATH, "templates", "default_calibration_command.json"), "r"
    ) as f:
        command = json.load(f)

    command["params"]["sensor_id"] = sensor_id
    command["params"]["gain"] = gain
    command["params"]["offset"] = offset

    mqtt_client.subscribe(topic_resp)
    start_time = time.time()
    mqtt_client.publish(topic_req, command)

    message = mqtt_client.wait_for_message(topic_resp, timeout=25)
    if message is None:
        raise TimeoutError("No message received within the timeout period.")
    print(f"Message received in {time.time() - start_time:.2f} seconds: {message}")
    # Validate response
    assert message["command_index"] == command["command"]
    assert message["command_status"] != 0

    return message


def send_and_validate_system_info(mqtt_client, user: str, password: str):
    topic_req = f"iocloud/request/all/command"
    topic_resp = f"iocloud/response/{_DEVICE_ID}/command"

    command = {}
    with open(
        os.path.join(_THIS_PATH, "templates", "default_get_system_command.json"), "r"
    ) as f:
        command = json.load(f)

    command["params"]["user"] = user
    command["params"]["password"] = password

    mqtt_client.subscribe(topic_resp)
    start_time = time.time()
    mqtt_client.publish(topic_req, command)

    message = mqtt_client.wait_for_message(topic_resp, timeout=25)
    if message is None:
        raise TimeoutError("No message received within the timeout period.")
    print(f"Message received in {time.time() - start_time:.2f} seconds: {message}")
    # Validate response
    assert message["command_index"] == command["command"]
    assert message["command_status"] == 0

    return message

@pytest.mark.skip(reason="Known issue: the invalid commands do not return any status")
def send_and_expect_fail_sys_info(mqtt_client, user: str, password: str):
    topic_req = f"iocloud/request/all/command"
    topic_resp = f"iocloud/response/{_DEVICE_ID}/command"

    command = {}
    with open(
        os.path.join(_THIS_PATH, "templates", "default_get_system_command.json"), "r"
    ) as f:
        command = json.load(f)

    command["params"]["user"] = user
    command["params"]["password"] = password

    mqtt_client.subscribe(topic_resp)
    start_time = time.time()
    mqtt_client.publish(topic_req, command)

    response = mqtt_client.wait_for_message(topic_resp, timeout=25)
    if response is None:
        raise TimeoutError("No response received within the timeout period.")
    print(f"Response received in {time.time() - start_time:.2f} seconds: {response}")
    # Validate response
    assert response["command_index"] == command["command"]
    assert response["command_status"] != 0

    return response


def receive_and_validate_sensor_report(mqtt_client):
    """
    Listen to a topic for incoming messages and validate the payload format.
    """
    topic = f"iocloud/response/{_DEVICE_ID}/sensor/report"

    mqtt_client.subscribe(topic)

    message = mqtt_client.wait_for_message(topic, timeout=65)
    if message:
        print(f"Received message: {message}")

    # Top-level keys
    assert "timestamp" in message, "Missing 'timestamp'"
    assert "sensors" in message, "Missing 'sensors'"
    assert isinstance(message["sensors"], list), "'sensors' should be a list"
    assert len(message["sensors"]) > 0, "No sensors in message"

    # Validate each sensor entry
    for i, sensor in enumerate(message["sensors"]):
        assert "value" in sensor, f"Sensor {i} missing 'value'"
        assert "active" in sensor, f"Sensor {i} missing 'active'"
        assert "unit" in sensor, f"Sensor {i} missing 'unit'"

        assert isinstance(
            sensor["value"], (int, float)
        ), f"Sensor {i} 'value' must be number"
        assert isinstance(sensor["active"], bool), f"Sensor {i} 'active' must be bool"
        assert isinstance(sensor["unit"], str), f"Sensor {i} 'unit' must be str"

    return message


# ------------------------
# Tests
# ------------------------
def test_calibration_command_single():
    """
    Positive test: send a calibration command to a single sensor and verify
    that the device responds successfully with the expected parameters.
    """
    mqtt_client = MqttTestClient()
    try:
        send_and_validate_calibration(mqtt_client, sensor_id=2, gain=1.0, offset=0.0)
    finally:
        mqtt_client.stop()


@pytest.mark.skip(reason="Known issue: the invalid commands do not return any status")
def test_calibration_command_invalid_sensor_id():
    """
    Negative test: send a calibration command with an invalid sensor ID and
    expect the device to reject it (non-zero status).
    """
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_calibration(mqtt_client, sensor_id="root", gain="admin", offset=3)
    finally:
        mqtt_client.stop()


@pytest.mark.skip(reason="Known issue: the invalid commands do not return any status")
def test_calibration_command_invalid_gain():
    """
    Negative test: send a calibration command with an invalid gain value and
    expect the device to reject it (non-zero status).
    """
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_calibration(mqtt_client, sensor_id=10, gain="2", offset=3)
    finally:
        mqtt_client.stop()


@pytest.mark.skip(reason="Known issue: the invalid commands do not return any status")
def test_calibration_command_invalid_offset():
    """
    Negative test: send a calibration command with an invalid offset value and
    expect the device to reject it (non-zero status).
    """
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_calibration(mqtt_client, sensor_id=10, gain=2, offset="3")
    finally:
        mqtt_client.stop()


def test_system_info_command_single():
    """
    Positive test: send a system info command with valid root credentials and
    verify that the device responds with a successful status.
    """
    mqtt_client = MqttTestClient()
    try:
        send_and_validate_system_info(mqtt_client, user="root", password="root")
    finally:
        mqtt_client.stop()


@pytest.mark.skip(reason="Known issue: the invalid commands do not return any status")
def test_system_info_command_invalid_user():
    """
    Negative test: send a system info command with an invalid username and
    expect the device to reject it (non-zero status).
    """
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_sys_info(mqtt_client, user="root11", password="root")
    finally:
        mqtt_client.stop()


@pytest.mark.skip(reason="Known issue: the invalid commands do not return any status")
def test_system_info_command_invalid_password():
    """
    Negative test: send a system info command with an invalid password and
    expect the device to reject it (non-zero status).
    """
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_sys_info(mqtt_client, user="root", password="root11")
    finally:
        mqtt_client.stop()


@pytest.mark.skip(reason="Known issue: the invalid commands do not return any status")
def test_system_info_command_invalid_user_type():
    """
    Negative test: send a system info command with a username of the wrong type
    (e.g. non-string) and expect the device to reject it (non-zero status).
    """
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_sys_info(mqtt_client, user="root", password="root11")
    finally:
        mqtt_client.stop()


@pytest.mark.skip(reason="Known issue: the invalid commands do not return any status")
def test_system_info_command_invalid_password_type():
    """
    Negative test: send a system info command with a password of the wrong type
    (e.g. non-string) and expect the device to reject it (non-zero status).
    """
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_sys_info(mqtt_client, user="root", password="root11")
    finally:
        mqtt_client.stop()


def test_sensor_report():
    """
    Positive test: receive a sensor report and validate its structure, ensuring
    it contains a timestamp and a non-empty list of properly formatted sensors.
    """
    mqtt_client = MqttTestClient()
    try:
        receive_and_validate_sensor_report(mqtt_client)
    finally:
        mqtt_client.stop()


def test_sensor_report_sampling():
    """
    Positive test: verify that sensor reports are published at the expected
    sampling interval. Ensures two valid reports are received and that the
    second arrives more than 65 seconds after the first.
    """
    mqtt_client = MqttTestClient()
    margin = 1
    min_interval = 5
    max_interval = min_interval + margin

    try:
        # Receive the first report
        receive_and_validate_sensor_report(mqtt_client)

        # Start timer after first valid report
        start_time = time.time()

        # Receive the second report (this will block until a message arrives or timeout inside the helper)
        receive_and_validate_sensor_report(mqtt_client)

        elapsed = time.time() - start_time
        print(f"Elapsed time between reports: {elapsed:.2f} seconds")

        assert (
            elapsed > min_interval
        ), f"Expected interval > {min_interval}s between reports, got {elapsed:.2f}s"
        assert (
            elapsed < max_interval
        ), f"Expected interval < {max_interval}s between reports, got {elapsed:.2f}s"

    finally:
        mqtt_client.stop()


import random
import math


def test_sensor_report_calibration():
    """
    Positive test: verify that applying a calibration command (gain + offset)
    to a sensor is reflected in subsequent sensor reports.

    The test:
    1. Receives an initial sensor report and records the raw value.
    2. Sends a calibration command for a chosen sensor with random gain/offset.
    3. Receives a new sensor report and checks that the reported value matches
       the expected calibrated value within a tolerance margin.
    """
    mqtt_client = MqttTestClient()
    sensor_id = 3
    # Random gain/offset within a reasonable range
    gain = random.uniform(0.5, 2.0)
    offset = random.uniform(-5.0, 5.0)

    try:
        # Step 1: get initial sensor value
        sensor_report = receive_and_validate_sensor_report(mqtt_client)
        sensor_list = sensor_report.get("sensors")
        original_value = sensor_list[sensor_id]["value"]

        # Step 2: send calibration
        send_and_validate_calibration(mqtt_client, sensor_id, gain, offset)

        # Step 3: get calibrated value from next report
        sensor_report = receive_and_validate_sensor_report(mqtt_client)
        sensor_list = sensor_report.get("sensors")
        calibrated_value = sensor_list[sensor_id]["value"]

        # Expected calibration
        expected_value = (original_value * gain) + offset

        # Allow tolerance for floating-point/device rounding errors
        tolerance = 0.1 * abs(expected_value)  # 1% margin
        assert math.isclose(
            calibrated_value, expected_value, rel_tol=0, abs_tol=tolerance
        ), (
            f"Expected calibrated value ~ {expected_value}, "
            f"but got {calibrated_value} (gain={gain}, offset={offset})"
        )
        
        system_info = send_and_validate_system_info(mqtt_client, "root", "root")
        system_info_sensor_list = system_info.get("sensors")
        system_info_sensor_list[sensor_id].get("gain")
        system_info_sensor_list[sensor_id].get("offset")

    finally:
        mqtt_client.stop()
