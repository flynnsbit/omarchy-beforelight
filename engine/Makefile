CC = gcc
CFLAGS = -Wall -Wextra -O2 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_image -lm
BUILD ?= build
PREBUILT = ../prebuilt
BINARIES = bouncingball fadeout fishsaver globe hardrain lifeforms lifeforms_new logo matrix messages messages2 paperfire rainstorm randomizer spotlight starrynight toastersaver warp worms

.PHONY: all clean dist
all: $(BUILD) fishsaver hardrain bouncingball globe warp toastersaver messages messages2 logo rainstorm spotlight lifeforms lifeforms_new fadeout matrix randomizer paperfire worms starrynight

$(BUILD):
	mkdir -p $(BUILD)

fishsaver: main_fish.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/fishsaver main_fish.c $(LDFLAGS)

hardrain: main_hard_rain.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/hardrain main_hard_rain.c $(LDFLAGS)

bouncingball: main_bouncing_ball.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/bouncingball main_bouncing_ball.c $(LDFLAGS)

globe: main_globe.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/globe main_globe.c $(LDFLAGS)

warp: main_warp.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/warp main_warp.c $(LDFLAGS)

toastersaver: main_toaster.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/toastersaver main_toaster.c $(LDFLAGS)

messages: main_messages.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/messages main_messages.c $(LDFLAGS) -lSDL2_ttf

messages2: main_messages2.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/messages2 main_messages2.c $(LDFLAGS) -lSDL2_ttf

logo: main_logo.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/logo main_logo.c $(LDFLAGS)

rainstorm: main_rainstorm.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/rainstorm main_rainstorm.c $(LDFLAGS)

spotlight: main_spotlight.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/spotlight main_spotlight.c $(LDFLAGS)

lifeforms: main_lifeforms_new.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/lifeforms main_lifeforms_new.c $(LDFLAGS)

lifeforms_new: lifeforms
	cp -a $(BUILD)/lifeforms $(BUILD)/lifeforms_new

fadeout: main_fadeout.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/fadeout main_fadeout.c $(LDFLAGS)

matrix: main_matrix.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/matrix main_matrix.c $(LDFLAGS) -lSDL2_ttf

randomizer: main_randomizer.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/randomizer main_randomizer.c $(LDFLAGS) -lSDL2_ttf

paperfire: main_paperfire.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/paperfire main_paperfire.c $(LDFLAGS)

worms: main_worms.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/worms main_worms.c $(LDFLAGS) -lSDL2_ttf -lSDL2_mixer

starrynight: starrynight.c starry_gl_sdl.h | $(BUILD)
	$(CC) $(CFLAGS) -Wno-unused-function -o $(BUILD)/starrynight starrynight.c $(LDFLAGS)

dist: all
	$(MAKE) -C overlay
	mkdir -p $(PREBUILT)
	cp -a $(addprefix $(BUILD)/,$(BINARIES)) overlay/libbeforelight-overlay.so $(PREBUILT)/
	cd $(PREBUILT) && sha256sum $(BINARIES) libbeforelight-overlay.so > SHA256SUMS

clean:
	rm -rf $(BUILD)
