# Instruction set for installing Piper TTS on Your Raspberry Pi
This document's purpose is to assist the reader on installing Piper TTS package on Raspberry Pi 4 and using it with OpenAi's Whisper.

#### Before continuing with the tutorial, make sure You are connected to Your Raspberry Pi 4 via SSH and have set up a virtual environment (`source env/bin/activate`)

### Step 1
Update Raspberry Pi:
```
sudo apt update && sudo apt upgrade -y

```
### Step 2
Make sure You have Python3 and ffmpeg (if not, they can be both installed by `sudo apt install python3 python3-pip ffmpeg -y`
Install Piper
```
pip install piper-tts
```
If `piper --help` shows a help message, Piper's installation was successful.

### Step 3
Download the Piper library:
```
wget https://github.com/rhasspy/piper/releases/download/v1.2.0/piper_arm64.tar.gz
```
When installed, convert the .tar file into a regular subfolder:
```
tar -xvzf piper_arm64.tar.gz
```

Now You should have a new subfolder, before proceeding to the next step, navigate into the new piper subfolder

### Step 4
Piper requires You to download the voices before You can properly use Piper. For this, run the code:
```
wget https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/amy/medium/en_US-amy-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/amy/medium/en_US-amy-medium.onnx.json
```
Right now we use voice Amy and the quality is medium. You can choose between many voices, you can check them out here: https://huggingface.co/rhasspy/piper-voices/tree/main/en/en_US

**IMPORTANT!** 

You should only use voice sizes low or medium because high package is too heavy for Raspberry Pi to use. You can choose the voice from huggingface but make sure you install them via the command line like we did in **Step 4**.

### Step 5
Check if the voice and Piper TTS works:
```
echo 'Welcome to the world of speech synthesis!' | ./piper --model en_US-amy-medium.onnx --output file welcome.wav
```

### Step 6
Proceed if Step 5 was successful.
Now it's time to test how we can bring OpenAi's Whisper and Piper TTS together and use them in a Python code. The Python code asks for your verbal input, uses Whisper to process it from STT and then processes it with Piper to TTS and plays it to the output device.
```
import whisper
import sounddevice as sd
import numpy as np
import wave
import os
import subprocess

# Load Whisper model (tiny is best for Raspberry Pi)
model = whisper.load_model("tiny")

# Set the Piper model path
PIPER_MODEL = "~/.local/share/piper/voices/en_us/en_US-ryan-low.onnx"

# Function to record audio
def record_audio(filename, duration=5, samplerate=16000):
    print("🎤 Recording... Speak now!")
    audio_data = sd.rec(int(duration * samplerate), samplerate=samplerate, channels=1, dtype=np.int16)
    sd.wait()
    
    # Save to WAV file
    wavefile = wave.open(filename, 'wb')
    wavefile.setnchannels(1)
    wavefile.setsampwidth(2)
    wavefile.setframerate(samplerate)
    wavefile.writeframes(audio_data)
    wavefile.close()
    print("✅ Recording saved as", filename)

# Function to transcribe speech to text
def speech_to_text(audio_file):
    print("📝 Transcribing...")
    result = model.transcribe(audio_file)
    text = result["text"]
    print("🗣 Recognized Speech:", text)
    return text

# Function to convert text to speech using Piper
def text_to_speech(text, output_wav="output.wav"):
    print("🎙 Generating speech with Piper...")
    command = f'echo "{text}" | piper -m {PIPER_MODEL} -o {output_wav}'
    os.system(command)
    print("🔊 Playing output...")
    subprocess.run(["aplay", output_wav])

# Main loop
def main():
    while True:
        input("Press Enter to start recording...")
        record_audio("input.wav")  # Record audio
        text = speech_to_text("input.wav")  # Convert to text
        if text.strip():
            text_to_speech(text)  # Convert text to speech

if __name__ == "__main__":
    main()

```
You should have the required packages installed from Adafruit Voice Bonnet driver installations but if You don't, then do it now. If there is an error, the error message tells you which package needs to be installed.

There You go!
Feel free to play around and implement Whsiper and Piper TTS to Your required python code.
