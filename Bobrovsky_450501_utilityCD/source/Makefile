CC = gcc
all:
	gcc `pkg-config --cflags gtk+-3.0` -o gtk gtk.c `pkg-config --libs gtk+-3.0`
	gcc -o blank blank.c
	gcc -o iso iso_write.c
	gcc -o au write_audio.c
	gcc -o conf get_conf.c

