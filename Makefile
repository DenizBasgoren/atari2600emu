all:
	gcc src/frontend.c src/cpu.c src/memory.c src/video.c src/timer.c src/audio.c src/input.c src/delayed_writer.c -lraylib -lm -O3 -g -o atari2600emu
