pico2wave -w tts.wav "$1"
aplay -D default tts.wav
rm -rf tts.wav


