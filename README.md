# TECHIN515_Team14

<img width="1274" alt="Screenshot 2024-05-28 at 14 47 15" src="https://github.com/qianq826/TECHIN515_Team14/assets/148395429/4c765105-43b1-4fcb-b5c2-70e898760b23">

### Circadian Floor lamp
Group 14:
Sebastian Qian | Yulin Li | Chaney He |


# Circadian Weather App Dashboard

This project is a Streamlit-based web application that adjusts LED colors based on circadian rhythms and weather conditions. It integrates with an ESP32 device via Azure IoT Hub to control the LED colors dynamically.

## Features

- Geocodes user-input location to fetch latitude and longitude.
- Fetches current weather conditions using weather.gov API.
- Determines the local time based on the geocoded location.
- Adjusts LED colors based on time of day and weather conditions.
- Provides a gradient of colors for the entire day.
- Allows manual control of LED colors via a color picker.

## Setup and Installation

### Prerequisites

- Python 3.6 or higher
- Streamlit
- Requests
- TimezoneFinder
- Pytz
- PIL (Pillow)
- Azure IoT Hub SDK for Python

### Installation

1. Clone this repository:
    ```sh
    git clone https://github.com/yourusername/circadian-weather-app.git
    cd circadian-weather-app
    ```

2. Install the required Python packages:
    ```sh
    pip install -r requirements.txt
    ```

3. Add your Azure IoT Hub connection string and device ID to the script:
    ```python
    connection_string = "HostName=your-iothub-connection-string"
    device_id = "your-device-id"
    ```

4. Add your custom logo image to the `img` directory.

### Running the Application

Run the Streamlit application:
```sh
streamlit run app.py







