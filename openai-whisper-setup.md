# Open-AI Whisper Setup on Raspberry Pi Debian.

#### 1) ```source env/bin/activate```
first of all, when You connect to the RasPi, set up a virtual environment
#### 2) ```pip install -U openai-whisper```
#### 3) install ffmpeg
In order for the setup to work, You will need a command-line tool ffmpeg. install it by running `sudo apt update && sudo apt install ffmpeg`.

Available sizes and models:
| Size | Parameters | English-only model | Multilingual model | Required VRAM | Relative speed |
|-------|-------|-------|-------|-------|-------|
| tiny    | 39 M   | `tiny.en`    | `tiny`    | ~1 GB    | ~10x    |
| base    | 74 M  | `base.en`   | `base`   | ~1 GB    | ~7x    |
| small    | 244 M    | `small.en`   | `small`    | ~2 GB    | ~4x    |
| medium    | 769 M    | `medium.en`    | `medium`    | ~5 GB    | ~2x    |
| large    | 1550 M    | N/A    | `large`    | ~10 GB    | ~1x    |
| turbo    | 809 M    | N/A   | `turbo`    | ~6 GB    | ~8x    |

In our instance, using the Raspberry Pi 4, the best model would be turbo, since we can go up to 7GB VRAM on the RasPi 4, which has 8GB of RAM.

### A basic Python code example to test transcription and detecting speech: 
```
import whisper

model = whisper.load_model("turbo")

# load audio and pad/trim it to fit 30 seconds
audio = whisper.load_audio("audio.mp3")
audio = whisper.pad_or_trim(audio)

# make log-Mel spectrogram and move to the same device as the model
mel = whisper.log_mel_spectrogram(audio).to(model.device)

# detect the spoken language
_, probs = model.detect_language(mel)
print(f"Detected language: {max(probs, key=probs.get)}")

# decode the audio
options = whisper.DecodingOptions()
result = whisper.decode(model, mel, options)

# print the recognized text
print(result.text)
```
#### Use `whisper-help` to get more familiar with the commands and feel free to then implement the model into your code.
