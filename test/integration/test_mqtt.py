import json
import logging
import math
import os
import pytest
import random
import time

from mqtt_module import MqttTestClient


def pytest_configure(config):
    """
    Ensures pytest logs show DEBUG output only when verbosity is enabled.
    """
    log_level = logging.INFO

    if config.getoption("verbose") > 0 or config.getoption("--log-cli-level", None) == "DEBUG":
        log_level = logging.DEBUG

    logging.basicConfig(
        level=log_level,
        format="%(asctime)s [%(levelname)s] %(message)s",
        datefmt="%H:%M:%S",
    )


_DEVICE_ID = "1C69209DB778"
# _DEVICE_ID = "1C69209DFC08"
_THIS_PATH = os.path.dirname(os.path.abspath(__file__))

def send_and_validate_calibration(mqtt_client, sensor_id: int, gain: float, offset: float):
    """
    Sends a calibration command for a given sensor and validates the response.

    Args:
        mqtt_client: The MQTT client used for publishing and subscribing.
        sensor_id (int): The ID of the sensor to calibrate.
        gain (float): The gain value to apply to the sensor.
        offset (float): The offset value to apply to the sensor.

    Returns:
        dict: The received MQTT response message.

    Raises:
        TimeoutError: If no response message is received within the timeout period.
        AssertionError: If the received message does not match the expected values.
    """
    topic_req = f"iocloud/request/{_DEVICE_ID}/command"
    topic_resp = f"iocloud/response/{_DEVICE_ID}/command"

    with open(os.path.join(_THIS_PATH, "templates", "default_calibration_command.json"), "r") as f:
        command = json.load(f)

    command["params"]["sensor_id"] = sensor_id
    command["params"]["gain"] = gain
    command["params"]["offset"] = offset

    mqtt_client.subscribe(topic_resp)

    start_time = time.time()
    mqtt_client.publish(topic_req, command)

    retries = 5
    message = None

    while retries > 0:
        try:
            message = mqtt_client.wait_for_message(topic_resp, timeout=25)
            if message is None:
                raise TimeoutError("No message received within the timeout period.")

            logging.debug(f"Message received in {time.time() - start_time:.2f} seconds: {message}")

            # Validate message content
            assert message["command_index"] == command["command"], (
                f"Expected command_index={command['command']}, got {message['command_index']}"
            )
            assert message["command_status"] == 0, (
                f"Expected command_status=0, got {message['command_status']}"
            )
            assert message["sensor_id"] == sensor_id, (
                f"Expected sensor_id={sensor_id}, got {message['sensor_id']}"
            )

            assert math.isclose(message["gain"], gain, rel_tol=1e-2), (
                f"Expected gain≈{gain:.3f}, got {message['gain']:.3f}"
            )
            assert math.isclose(message["offset"], offset, rel_tol=1e-2), (
                f"Expected offset≈{offset:.3f}, got {message['offset']:.3f}"
            )

            # If all assertions pass, break out of retry loop
            break

        except Exception as e:
            logging.debug(f"Validation attempt failed: {e}")
            retries -= 1
            if retries == 0:
                raise
            logging.debug(f"Retrying... ({retries} attempts left)")
            time.sleep(1)

    return message

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
    logging.debug(f"Message received in {time.time() - start_time:.2f} seconds: {message}")
    # Validate response
    assert message["command_index"] == command["command"]
    assert message["command_status"] != 0

    return message

def send_and_validate_system_info(mqtt_client, user: str, password: str, success: bool = True):
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

    message = None
    if success:
        message = mqtt_client.wait_for_message(topic_resp, timeout=25)
        if message is None:
            raise TimeoutError("No message received within the timeout period.")
        logging.debug(f"Message received in {time.time() - start_time:.2f} seconds: {message}")

        assert message["command_index"] == command["command"]
        assert message["command_status"] == 0

    return message

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
    logging.debug(f"Response received in {time.time() - start_time:.2f} seconds: {response}")
    # Validate response
    assert response["command_index"] == command["command"]
    assert response["command_status"] != 0

    return response

def receive_and_validate_sensor_report(mqtt_client, clear: bool = False):
    """
    Listen to a topic for incoming messages and validate the payload format.
    """
    topic = f"iocloud/response/{_DEVICE_ID}/sensor/report"

    if clear:
        mqtt_client.clear_message_list(topic)

    mqtt_client.subscribe(topic)

    message = mqtt_client.wait_for_message(topic, timeout=65)
    if message:
        logging.debug(f"Received message: {message}")

    assert "timestamp" in message, "Missing 'timestamp'"
    assert "sensors" in message, "Missing 'sensors'"
    assert isinstance(message["sensors"], list), "'sensors' should be a list"
    assert len(message["sensors"]) > 0, "No sensors in message"

    for i, sensor in enumerate(message["sensors"]):
        assert "value" in sensor, f"Sensor {i} missing 'value'"
        assert "active" in sensor, f"Sensor {i} missing 'active'"

        assert isinstance(
            sensor["value"], (int, float)
        ), f"Sensor {i} 'value' must be number"
        assert isinstance(sensor["active"], bool), f"Sensor {i} 'active' must be bool"

    return message


# ------------------------
# Tests
# ------------------------
@pytest.mark.high
def test_calibration_command_single():
    """
    Validates that a calibration command sent to a single sensor succeeds.

    Steps:
    1. Send a calibration command with valid gain and offset parameters.
    2. Wait for a response from the device.
    3. Confirm that the device returns command_status=0 and matching parameters.
    """
    logging.info(test_calibration_command_single.__doc__)
    
    mqtt_client = MqttTestClient()
    try:
        send_and_validate_calibration(mqtt_client, sensor_id=2, gain=1.0, offset=0.0)
    finally:
        mqtt_client.stop()

@pytest.mark.high
def test_calibration_command_invalid_sensor_id():
    """
    Negative test: verify that the device correctly rejects calibration requests 
    with an invalid (non-integer) sensor ID.

    Steps:
    1. Send a calibration command using an invalid string-based sensor_id.
    2. Wait for the device's response.
    3. Confirm that the response command_status is non-zero (indicating rejection).
    """
    logging.info(test_calibration_command_invalid_sensor_id.__doc__)
    
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_calibration(mqtt_client, sensor_id="root", gain="admin", offset=3)
    finally:
        mqtt_client.stop()


@pytest.mark.high
def test_calibration_command_invalid_gain():
    """
    Negative test: verify that sending a calibration command with an invalid gain 
    type (string instead of numeric) fails.

    Steps:
    1. Send a calibration command with a string-based gain parameter.
    2. Wait for the device's response.
    3. Verify that the response contains a non-zero command_status.
    """
    logging.info(test_calibration_command_invalid_gain.__doc__)
    
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_calibration(mqtt_client, sensor_id=10, gain="2", offset=3)
    finally:
        mqtt_client.stop()


@pytest.mark.high
def test_calibration_command_invalid_offset():
    """
    Negative test: verify that sending a calibration command with an invalid offset 
    type (string instead of numeric) fails.

    Steps:
    1. Send a calibration command with a string-based offset parameter.
    2. Wait for the device's response.
    3. Confirm that command_status is non-zero (indicating rejection).
    """
    logging.info(test_calibration_command_invalid_offset.__doc__)
    
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_calibration(mqtt_client, sensor_id=10, gain=2, offset="3")
    finally:
        mqtt_client.stop()

@pytest.mark.high
def test_system_info_command_single():
    """
    Positive test: verify that a system info command using valid root credentials 
    succeeds.

    Steps:
    1. Send a system info command with user="root" and password="root".
    2. Wait for the device's response.
    3. Validate that command_status equals 0 and the response structure is correct.
    """
    logging.info(test_system_info_command_single.__doc__)
    
    mqtt_client = MqttTestClient()
    try:
        send_and_validate_system_info(mqtt_client, user="root", password="root")
    finally:
        mqtt_client.stop()


@pytest.mark.high
def test_system_info_command_invalid_user():
    """
    Negative test: verify that the system info command fails when provided 
    with an invalid username.

    Steps:
    1. Send a system info command with an invalid username.
    2. Wait for the device’s response.
    3. Validate that command_status is non-zero (authentication failure).
    """
    logging.info(test_system_info_command_invalid_user.__doc__)
    
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_sys_info(mqtt_client, user="root11", password="root")
    finally:
        mqtt_client.stop()


@pytest.mark.high
def test_system_info_command_invalid_password():
    """
    Negative test: verify that the system info command fails when provided 
    with an invalid password.

    Steps:
    1. Send a system info command using the correct username but wrong password.
    2. Wait for the response.
    3. Ensure that the command_status is non-zero (access denied).
    """
    logging.info(test_system_info_command_invalid_password.__doc__)
    
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_sys_info(mqtt_client, user="root", password="root11")
    finally:
        mqtt_client.stop()


@pytest.mark.high
def test_system_info_command_invalid_user_type():
    """
    Negative test: verify that using a non-string username type in the system 
    info command triggers a failure.

    Steps:
    1. Send a system info command where 'user' is an integer instead of a string.
    2. Wait for the response from the device.
    3. Confirm that command_status is non-zero (invalid parameter type).
    """
    logging.info(test_system_info_command_invalid_user_type.__doc__)
    
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_sys_info(mqtt_client, user="root", password="root11")
    finally:
        mqtt_client.stop()


@pytest.mark.high
def test_system_info_command_invalid_password_type():
    """
    Negative test: verify that using a non-string password type in the system 
    info command triggers a failure.

    Steps:
    1. Send a system info command where 'password' is an integer instead of a string.
    2. Wait for the device's response.
    3. Confirm that command_status is non-zero (invalid parameter type).
    """
    logging.info(test_system_info_command_invalid_password_type.__doc__)
    
    mqtt_client = MqttTestClient()
    try:
        send_and_expect_fail_sys_info(mqtt_client, user="root", password="root11")
    finally:
        mqtt_client.stop()

@pytest.mark.high
def test_sensor_report():
    """
    Positive test: verify that a sensor report is received and structured correctly.

    Steps:
    1. Subscribe to the sensor report topic.
    2. Wait for the incoming message.
    3. Validate that:
       - The payload includes a timestamp.
       - The 'sensors' list is non-empty.
       - Each sensor entry has valid 'value' (number) and 'active' (bool) fields.
    """
    logging.info(test_sensor_report.__doc__)
    
    mqtt_client = MqttTestClient()
    try:
        receive_and_validate_sensor_report(mqtt_client)
    finally:
        mqtt_client.stop()

@pytest.mark.high
def test_sensor_report_sampling():
    """
    Positive test: verify that sensor reports are published at the expected
    sampling interval. Ensures two valid reports are received and that the
    second arrives more than 65 seconds after the first.
    """
    logging.info(test_sensor_report_sampling.__doc__)
    
    mqtt_client = MqttTestClient()
    margin = 1
    min_interval = 5
    max_interval = min_interval + margin

    try:
        first_report = receive_and_validate_sensor_report(mqtt_client, clear=True)
        second_report = receive_and_validate_sensor_report(mqtt_client, clear=True)

        start_ts = first_report["timestamp"]
        end_ts = second_report["timestamp"]

        if end_ts > 1e12:
            start_ts /= 1000
            end_ts /= 1000

        elapsed = end_ts - start_ts
        logging.debug(f"Elapsed time between reports: {elapsed:.2f} seconds")

        assert elapsed >= min_interval, (
            f"Expected interval > {min_interval}s, got {elapsed:.2f}s"
        )
        assert elapsed < max_interval, (
            f"Expected interval < {max_interval}s, got {elapsed:.2f}s"
        )

    finally:
        mqtt_client.stop()


@pytest.mark.medium
def test_sensor_report_calibration():
    """
    Positive test: verify that applying a calibration command (gain + offset)
    to a sensor is reflected in subsequent sensor reports.

    Steps:
    1. Receive an initial sensor report and record each sensor’s raw value.
    2. Send calibration commands with random gain and offset for every sensor.
    3. Verify the calibration parameters are updated in system info.
    4. Receive a post-calibration sensor report.
    5. Validate that each active sensor’s reported value matches the expected
       calibrated value within an acceptable tolerance.
    """
    logging.info(test_sensor_report_calibration.__doc__)
    
    mqtt_client = MqttTestClient()
    gain_list = []
    offset_list = []
    try:
        logging.debug("===== STEP 1: Receiving Pre-Calibration Sensor Report =====")
        pre_calibration_sensor_report = receive_and_validate_sensor_report(mqtt_client)
        pre_calibration_sensor_list = pre_calibration_sensor_report.get("sensors", [])

        logging.debug(f"Number of sensors found: {len(pre_calibration_sensor_list)}")
        system_info = send_and_validate_system_info(mqtt_client, "root", "root")
        system_info_sensor_list = system_info.get("sensors", [])
        for sensor_id, sensor in enumerate(system_info_sensor_list):
            gain = random.uniform(0.5, 2.0)
            offset = random.uniform(-5.0, 5.0)
            gain_list.append(gain)
            offset_list.append(offset)

            logging.debug(f"[Sensor {sensor_id}] Assigning calibration parameters: "
                        f"gain={gain:.3f}, offset={offset:.3f}")

            # Calibrate the sensor
            send_and_validate_calibration(mqtt_client, sensor_id, gain, offset)
            logging.debug(f"[Sensor {sensor_id}] Calibration command sent and validated successfully.")

        logging.debug("===== STEP 2: Verifying Calibration in System Info =====")
        system_info = send_and_validate_system_info(mqtt_client, "root", "root")
        logging.debug(system_info)
        system_info_sensor_list = system_info.get("sensors", [])

        logging.debug(f"Retrieved {len(system_info_sensor_list)} sensors from system info.")

        for sensor_id, sensor in enumerate(system_info_sensor_list):
            expected_gain = gain_list[sensor_id]
            expected_offset = offset_list[sensor_id]
            actual_gain = sensor.get("gain")
            actual_offset = sensor.get("offset")

            logging.debug(f"[Sensor {sensor_id}] Validating calibration results...")
            logging.debug(f"  Expected gain={expected_gain:.3f}, Reported gain={actual_gain:.3f}")
            logging.debug(f"  Expected offset={expected_offset:.3f}, Reported offset={actual_offset:.3f}")

            assert math.isclose(expected_gain, actual_gain, rel_tol=1e-2), (
                f"Expected gain≈{expected_gain}, got {actual_gain:.3f}"
            )
            assert math.isclose(expected_offset, actual_offset, rel_tol=1e-2), (
                f"Expected offset≈{expected_offset}, got {actual_offset:.3f}"
            )
        
        logging.debug("===== STEP 3: Verifying Calibration in System Info =====")

        # Receive a new sensor report after calibration
        post_calibration_sensor_report = receive_and_validate_sensor_report(mqtt_client, True)
        post_calibration_sensor_list = post_calibration_sensor_report.get("sensors", [])

        for sensor_id, sensor in enumerate(system_info_sensor_list):
            # Retrieve pre-calibration and post-calibration values
            original_value = pre_calibration_sensor_list[sensor_id].get("value")
            reported_value = post_calibration_sensor_list[sensor_id].get("value")

            # Compute expected calibrated value
            expected_gain = gain_list[sensor_id]
            expected_offset = offset_list[sensor_id]
            expected_value = (expected_gain * original_value) + expected_offset

            logging.debug(f"[Sensor {sensor_id}] Validating calibration results...")
            logging.debug(f"  Gain: expected {expected_gain:.3f}")
            logging.debug(f"  Offset: expected {expected_offset:.3f}")
            logging.debug(f"  Original value: {original_value:.3f}")
            logging.debug(f"  Expected calibrated value: {expected_value:.3f}")
            logging.debug(f"  Reported calibrated value: {reported_value:.3f}")
            if sensor.get("active"):
                # Validate calibrated value within tolerance
                assert math.isclose(expected_value, reported_value, rel_tol=0.5), (
                    f"[Sensor {sensor_id}] Calibration mismatch: expected≈{expected_value:.3f}, "
                    f"got {reported_value:.3f} (gain={expected_gain:.3f}, offset={expected_offset:.3f})"
                )

    except Exception as e:
        raise Exception(e)
    finally:
        mqtt_client.stop()
            
@pytest.mark.low
def test_stress_system():
    """
    Long-duration stability test: verify that repeated system info queries
    do not cause reboots or unplanned restarts.

    Steps:
    1. Initialize MQTT client and set counters for uptime, reboots, and timeouts.
    2. Loop through a large number of iterations (e.g., 5000):
       - Optionally send calibration commands (commented out for stability).
       - Request system info and track uptime progression.
    3. Confirm that uptime never decreases across iterations (no reboots).
    4. Record timeout and reboot counters for analysis.
    """
    logging.info(test_stress_system.__doc__)

    uptime = 0
    timeout_counter = 0
    iterations = 5000
    mqtt_client = MqttTestClient()
    reboot_count = 0
    start_time = time.time()
    
    for i in range(iterations):
        logging.debug(f"--- Iteration {i+1}/{iterations} --- Reboots: {reboot_count} --- Timeouts: {timeout_counter}")
        time.sleep(0.25)
        system_info = send_and_validate_system_info(mqtt_client, "root", "root")
        if i % 100 == 0:
            elapsed = time.time() - start_time
            logging.info(f"Progress: iteration {i}/{iterations} "
                            f"({elapsed/60:.1f} min elapsed)")
        
        if system_info is None:
            timeout_counter += 1
            continue
        
        last_valid_uptime = system_info.get("uptime")
        if uptime <= last_valid_uptime:
            uptime = last_valid_uptime
        else:
            reboot_count += 1
    mqtt_client.stop()

@pytest.mark.low
def test_stress_system_without_validate_command():
    """
    Positive test: verify that applying a calibration command (gain + offset)
    to a sensor is reflected in subsequent sensor reports.

    The test:
    1. Receives an initial sensor report and records the raw value.
    2. Sends a calibration command for a chosen sensor with random gain/offset.
    3. Receives a new sensor report and checks that the reported value matches
       the expected calibrated value within a tolerance margin.
    """
    logging.info(test_stress_system_without_validate_command.__doc__)
    
    iterations = 5000
    mqtt_client = MqttTestClient()
    start_time = time.time()
    
    for i in range(iterations):
        send_and_validate_system_info(mqtt_client, "root", "root", success=False)
        if i % 100 == 0:
            elapsed = time.time() - start_time
            logging.info(f"Progress: iteration {i}/{iterations} "
                            f"({elapsed/60:.1f} min elapsed)")
        time.sleep(0.1)
        
    post_calibration_sensor_report = receive_and_validate_sensor_report(mqtt_client, True)
    
    assert post_calibration_sensor_report is not None

    mqtt_client.stop()
