# Temperature-and-Humidity-Logger
An Arduino based program for logging the temperature and humidity using Adafruit Feather M0 Adalogger microcontroller, Adafruit FeatherWing OLED for display and user input, Thermocouple Amplifier MAX31855 to read thermocouples, and DHT22 humidity sensor. Alows user to choose a file name of the form AA00 and automatically increments based off of the previous file name. Saves temperature and or humidity of up to 10 sensors at once either on button press or continuously. User can scroll through current sensor readings for real time information. Useful in higvoltage enviroments because of the ability to float relative to ground and modular nature alows for quick and inexpensive repair of damaged circuitry. The microcontroler program can loaded onto a board from Temp_and_humid_log.ino using arduino IDE.
# DHT22 humidity sensor adapter board
<img width="1182" height="1181" alt="8677062374706397184-simulation_image_top" src="https://github.com/user-attachments/assets/7cef93ce-9c3f-44db-b4e8-116b1597284a" />
Designed to allow for connection of wires and incorporation of a 10kOhm pull up resistor

# Logger breakout board
Thanks to a colleague Gabriel Firesone for designing a breakout board for easily connecting 

<img width="1197" height="1041" alt="simulation_image_top" src="https://github.com/user-attachments/assets/dd9b1037-4b2b-4b49-b55b-84030cba762b" />

