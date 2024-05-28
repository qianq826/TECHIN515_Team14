import time
import json
import os
import traceback
import logging

from azure.iot.device import IoTHubDeviceClient, MethodResponse, exceptions
from dotenv import load_dotenv

# Load environment variables
load_dotenv()

IOTHUB_CONNECTION_STRING = "HostName=iotdevice-esp32.azure-devices.net;DeviceId=Esp32-Wroom;SharedAccessKey=N6efvK7/swVIE43XawwzR+/xQSB2dUg6cAIoTH0SkAs="
DEVICE_ID = "Esp32-Wroom"

# Print the connection string and device ID to verify they are loaded correctly
print(f"IoT Hub Connection String: {IOTHUB_CONNECTION_STRING}")
print(f"Device ID: {DEVICE_ID}")

# Check if the connection string and device ID are loaded correctly
if not IOTHUB_CONNECTION_STRING or "HostName" not in IOTHUB_CONNECTION_STRING:
    raise ValueError("IOTHUB_CONNECTION_STRING is not set correctly. Please check your .env file.")

if not DEVICE_ID:
    raise ValueError("DEVICE_ID is not set. Please check your .env file.")

logging.basicConfig(level=logging.ERROR)

waiting_for_next_message = True

def create_client():
    client = IoTHubDeviceClient.create_from_connection_string(IOTHUB_CONNECTION_STRING)
    client.on_message_received = message_received_handler
    return client

def message_received_handler(message):
    global waiting_for_next_message
    message_content = message.data.decode('utf-8')
    print(f"Message received: {message_content}")

    try:
        message_json = json.loads(message_content)
        if all(key in message_json for key in ("red", "green", "blue")):
            print("RGB message received. Waiting for the next message...")
            waiting_for_next_message = False
    except json.JSONDecodeError:
        print("Received message is not a valid JSON.")

client = create_client()

# Main loop with error handling and reconnection logic
while True:
    try:
        waiting_for_next_message = True
        print("Listening for messages...")
        while waiting_for_next_message:
            time.sleep(1)
    except exceptions.ConnectionDroppedError:
        print("Connection dropped. Attempting to reconnect...")
        while True:
            try:
                client.shutdown()
                client = create_client()
                print("Reconnected successfully.")
                break
            except exceptions.ConnectionFailedError as e:
                print(f"Reconnection failed: {e}. Retrying in 5 seconds...")
                time.sleep(5)
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        traceback.print_exc()