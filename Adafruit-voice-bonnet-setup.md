# Instructions for using Seeed Studio voice bonnets (Adafruit and Respeaker)


#### This document is created in the aim of helping users download the proper drivers needed for the Adafruit and/or Respeaker Voice Bonnet. The text which is in bold and has highlighted background (example) is meant to be pasted into the terminal command line. This document is created using info from Adafruit’s Voice Bonnet setup tutorial pages.
https://learn.adafruit.com/adafruit-voice-bonnet/raspberry-pi-setup

https://learn.adafruit.com/adafruit-voice-bonnet/blinka-setup

https://learn.adafruit.com/adafruit-voice-bonnet/audio-setup


## Setting up Python
### Step 1
First of all, make sure You have your Raspberry Pi set up and have connection to it.

### Step 2
Open the terminal in the RasPi and perform an update:
```
sudo apt update
```
```
sudo apt  -y upgrade
```

and download Python aswell:
```
sudo apt install --upgrade python3-setuptools
```

next up You should set up a Virtual Environment:
```
sudo apt install python3.11-venv
```
```
python -m venv env --system-site-packages
```

To activate the Virtual Environment run:
```
source env/bin/activate
```

To deactivate the Virtual Environment, simply run:
```
deactivate
```

## Setting up Blinka
### Step 3
Download Blinka:
```
cd ~
pip3 install --upgrade adafruit-python-shell
wget https://raw.githubusercontent.com/adafruit/Raspberry-Pi-Installer-Scripts/master/raspi-blinka.py
sudo -E env PATH=$PATH python3 raspi-blinka.py
```
When it asks you if you want to reboot, choose **yes**

### Step 4
Once it reboots, don’t forget to activate your virtual environment again!

Install the DotStar library, which is for controlling on-board LEDs and testing the GPIO pins:
```
pip3 install --upgrade adafruit-circuitpython-dotstar adafruit-circuitpython-motor adafruit-circuitpython-bmp280
```


## Setting up the Voicecard
### Step 5
Make sure You have the voice card and **I2C** support installed.
When you `run sudo i2cdetect -y 1` you should see an entry under 1A, indicating the hardware sees the audio card. The number may also appear as UU if you already installed software

![image](https://github.com/user-attachments/assets/dfe46fc5-faaf-4c69-aecd-ee18ac142885)


At the command line run:
```
cd ~
sudo apt-get install -y git
git clone https://github.com/HinTak/seeed-voicecard
cd seeed-voicecard
git checkout v6.6
sudo ./install.sh
```
At the end You should see something like this:

![image](https://github.com/user-attachments/assets/4dd7c825-ff92-4f1c-a5f0-4322130da1aa)

now reboot with `sudo reboot`

### Step 6
After the reboot run `sudo aplay -l` to list all cards, You should see it at the bottom

![image](https://github.com/user-attachments/assets/3dda38f0-2325-472e-a9c6-b4797bc0a562)

PS! Take note of the card number!

### Step 7
Adjustments with Alsamixer:
run `alsamixer`  to access the audio settings. Make sure to select the voice card with F6 and adjust accordingly.


## Speaker  test
### Step 8
Make sure the **Audio On / Off** is set to **On** on the voicecard.
Make sure you have headphones or a speaker attached to the speaker port. Then run
```
speaker-test-c2
```
You will hear white noise coming out of the speakers/headphones! If you don't hear anything, you may need to change the -c parameter to match the card number of your device.

## Microphone test
### Step 9
run 
```
sudo arecord -f cd -Dhw:3 | aplay -Dhw:3
```
PS! If your sound card is not #2, then replace both of the -Dhw: parameters with your actual number.
And then speak to the microphone, You should hear yourself echoed.
Once You’re done, press Control + C to stop the process.

## Python Usage
### Step 10
If you would like to use the audio in Python aswell (with the RSC you should) then install proper libraries for this:
```
sudo apt-get install libportaudio2 portaudio19-dev
sudo pip3 install pyaudio
```

### Step 11
Here is a basic test script to enumerate the devices and record for 10 seconds. When prompted, choose the device called **seeed-2mic-voicecard**.
Copy and paste the following code into a file called **audiotest.py**:
```
import pyaudio
import wave

FORMAT = pyaudio.paInt16
CHANNELS = 1       	# Number of channels
BITRATE = 44100    	# Audio Bitrate
CHUNK_SIZE = 512   	# Chunk size to
RECORDING_LENGTH = 10  # Recording Length in seconds
WAVE_OUTPUT_FILENAME = "myrecording.wav"
audio = pyaudio.PyAudio()

info = audio.get_host_api_info_by_index(0)
numdevices = info.get('deviceCount')
for i in range(0, numdevices):
	if (audio.get_device_info_by_host_api_device_index(0, i).get('maxInputChannels')) > 0:
    	print("Input Device id ", i, " - ", audio.get_device_info_by_host_api_device_index(0, i).get('name'))

print("Which Input Device would you like to use?")
device_id = int(input()) # Choose a device
print("Recording using Input Device ID "+str(device_id))

stream = audio.open(
	format=FORMAT,
	channels=CHANNELS,
	rate=BITRATE,
	input=True,
	input_device_index = device_id,
	frames_per_buffer=CHUNK_SIZE
)

recording_frames = []

for i in range(int(BITRATE / CHUNK_SIZE * RECORDING_LENGTH)):
	data = stream.read(CHUNK_SIZE)
	recording_frames.append(data)

stream.stop_stream()
stream.close()
audio.terminate()

waveFile = wave.open(WAVE_OUTPUT_FILENAME, 'wb')
waveFile.setnchannels(CHANNELS)
waveFile.setsampwidth(audio.get_sample_size(FORMAT))
waveFile.setframerate(BITRATE)
waveFile.writeframes(b''.join(recording_frames))
waveFile.close()


Run the code with the following command:
sudo python3 audiotest.py 

When finished, you can test playing back the file with the following command:
aplay myrecording.wav
```
