# Requirements:
- libavcodec
- libavformat
- libavutil
- libswscale

# How to instal them:
sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

# Compilation: you have to link the libraries above, including libm.
gcc file.c -o file -lavformat -lavcodec -lavutil -lm

# Compile
make

# Run
	./main in/small.mp4 out/small.mp4 blur
or
	./main videos/small.mp4 out/small_saturation.mp4 saturation 1.2 1.05 1.05
or just run ./main to print the usage.
