CC = gcc
CFLAGS = -Wall -Wextra -O2 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_image -lm

.PHONY: all clean
all: build fishsaver hardrain bouncingball globe warp toastersaver messages messages2 logo rainstorm spotlight lifeforms fadeout matrix randomizer paperfire worms starrynight

build:
	mkdir -p build

fishsaver: main_fish.c | build
	$(CC) $(CFLAGS) -o build/fishsaver main_fish.c $(LDFLAGS)

hardrain: main_hard_rain.c | build
	$(CC) $(CFLAGS) -o build/hardrain main_hard_rain.c $(LDFLAGS)

bouncingball: main_bouncing_ball.c | build
	$(CC) $(CFLAGS) -o build/bouncingball main_bouncing_ball.c $(LDFLAGS)

globe: main_globe.c | build
	$(CC) $(CFLAGS) -o build/globe main_globe.c $(LDFLAGS)

warp: main_warp.c | build
	$(CC) $(CFLAGS) -o build/warp main_warp.c $(LDFLAGS)

toastersaver: main_toaster.c | build
	$(CC) $(CFLAGS) -o build/toastersaver main_toaster.c $(LDFLAGS)

messages: main_messages.c | build
	$(CC) $(CFLAGS) -o build/messages main_messages.c $(LDFLAGS) -lSDL2_ttf

messages2: main_messages2.c | build
	$(CC) $(CFLAGS) -o build/messages2 main_messages2.c $(LDFLAGS) -lSDL2_ttf

logo: main_logo.c | build
	$(CC) $(CFLAGS) -o build/logo main_logo.c $(LDFLAGS)

rainstorm: main_rainstorm.c | build
	$(CC) $(CFLAGS) -o build/rainstorm main_rainstorm.c $(LDFLAGS)

spotlight: main_spotlight.c | build
	$(CC) $(CFLAGS) -o build/spotlight main_spotlight.c $(LDFLAGS)

lifeforms: main_lifeforms_new.c | build
	$(CC) $(CFLAGS) -o build/lifeforms main_lifeforms_new.c $(LDFLAGS)

fadeout: main_fadeout.c | build
	$(CC) $(CFLAGS) -o build/fadeout main_fadeout.c $(LDFLAGS)

matrix: main_matrix.c | build
	$(CC) $(CFLAGS) -o build/matrix main_matrix.c $(LDFLAGS) -lSDL2_ttf

randomizer: main_randomizer.c | build
	$(CC) $(CFLAGS) -o build/randomizer main_randomizer.c $(LDFLAGS) -lSDL2_ttf

paperfire: main_paperfire.c | build
	$(CC) $(CFLAGS) -o build/paperfire main_paperfire.c $(LDFLAGS)

worms: main_worms.c | build
	$(CC) $(CFLAGS) -o build/worms main_worms.c $(LDFLAGS) -lSDL2_ttf -lSDL2_mixer

starrynight: starrynight.c | build
	$(CC) $(CFLAGS) -o build/starrynight starrynight.c $(LDFLAGS) -lSDL2_ttf -lGL -lGLU

clean:
	rm -rf build
