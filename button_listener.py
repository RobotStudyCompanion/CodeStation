from aiy.board import Board, Led
import subprocess

def main():
    print('Press the button to run speak_gpt.py (Ctrl-C to exit).')
    script_path = '/home/pi/Desktop/RoboticStudyCompanion-main/CodeStation/speak_gpt.py'  # Full path to speak_gpt.py

    with Board() as board:
        while True:
            board.button.wait_for_press()
            print('Button pressed, running speak_gpt.py')
            subprocess.run(['sudo','python3', script_path])
            board.led.state = Led.ON
            board.button.wait_for_release()
            print('Button released')
            board.led.state = Led.OFF

if __name__ == '__main__':
    main()

# written by Isaac & Ishamael Jackson, May2024
'''
### Button_listener.py file configuration:
Ensure Your Script is Ready:
Make sure your button_listener.py script is executable and located in /home/pi/.

Create a systemd Service File:
Open a terminal on your Raspberry Pi and create a new service file:
bash
Copy code
sudo nano /etc/systemd/system/button_listener.service

Add the Following Configuration to the File:
ini
Copy code
[Unit]
Description=Button Listener Python Script
After=network.target
[Service]
ExecStart=/usr/bin/python3 /home/pi/button_listener.py
WorkingDirectory=/home/pi/
StandardOutput=inherit
StandardError=inherit
Restart=always
User=pi
Group=pi
[Install]
WantedBy=multi-user.target
ExecStart specifies the command to run your Python script. Adjust the path if your script is located elsewhere.
WorkingDirectory is set to /home/pi/ where your script is located.
StandardOutput and StandardError are set to inherit for logging.
Restart=always ensures the service restarts if the script fails.

Reload systemd to Recognize the New Service:
bash
Copy code
sudo systemctl daemon-reload

Enable the Service to Start on Boot:
bash
Copy code
sudo systemctl enable button_listener.service

Start the Service Immediately (Optional):
bash
Copy code
sudo systemctl start button_listener.service

Check the Status of the Service:
bash
Copy code
sudo systemctl status button_listener.service
This will show you the status and any output or errors from your script.
'''
