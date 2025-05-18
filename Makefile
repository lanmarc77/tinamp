CC_aarch64 = gcc
CC_amd64 = gcc
LDFLAGS = -lSDL2 -lSDL2_ttf -lvlc -Wl,--strip-debug
CFLAGS = -Wall -g

SRCS = md5.c saves.c format_helper.c ff_handling.c sd_play.c queue.c util.c battery.c display.c input.c SDL_FontCache.c ui_elements.c screens.c tinamp.c

BUILD_DIR = .build
TARGET_aarch64 = tinamp_aarch64
TARGET_amd64 = tinamp_amd64

OBJS_aarch64 = $(patsubst %.c, $(BUILD_DIR)/%.aarch64.o, $(SRCS))
OBJS_amd64 = $(patsubst %.c, $(BUILD_DIR)/%.amd64.o, $(SRCS))

all: $(TARGET_amd64)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


portmaster:
	-rm $(BUILD_DIR)/tinamp.zip
	-rm -r $(BUILD_DIR)/tinamp
	./buildPortmasterPackage.sh

libvlc_aarch64_compile:
	- GIT_SSL_NO_VERIFY=true git clone -b 3.0.x https://github.com/videolan/vlc $(BUILD_DIR)/vlc
	./compileLibvlcAarch64.sh

libffmpeg_aarch64_compile:
#	- GIT_SSL_NO_VERIFY=true git clone -b release/7.1 https://github.com/FFmpeg/FFmpeg $(BUILD_DIR)/FFmpeg
#	- GIT_SSL_NO_VERIFY=true git clone -b release/6.1 https://github.com/FFmpeg/FFmpeg $(BUILD_DIR)/FFmpeg
#	- GIT_SSL_NO_VERIFY=true git clone -b release/6.0 https://github.com/FFmpeg/FFmpeg $(BUILD_DIR)/FFmpeg
	- GIT_SSL_NO_VERIFY=true git clone -b release/4.4 https://github.com/FFmpeg/FFmpeg $(BUILD_DIR)/FFmpeg
	./compileFFmpegAarch64.sh

libvlc_aarch64: $(BUILD_DIR)
#	setup docker for arm64 emulation, if needed
	-docker run --privileged --rm tonistiigi/binfmt --install all
#	remove a maybe outdated build container
	-docker image rm cross-compile-tinamp
#	create a new build container
	docker buildx build --platform linux/arm64 -t cross-compile-tinamp .
#	start the arm64 build
	docker run --platform linux/arm64 --rm -v $(shell pwd):/build -w /build cross-compile-tinamp make libvlc_aarch64_compile

libffmpeg_aarch64: $(BUILD_DIR)
#	setup docker for arm64 emulation, if needed
	-docker run --privileged --rm tonistiigi/binfmt --install all
#	remove a maybe outdated build container
	-docker image rm cross-compile-tinamp
#	create a new build container
	docker buildx build --platform linux/arm64 -t cross-compile-tinamp .
#	start the arm64 build
	docker run --platform linux/arm64 --rm -v $(shell pwd):/build -w /build cross-compile-tinamp make libffmpeg_aarch64_compile

$(TARGET_aarch64): $(BUILD_DIR)
#	setup docker for arm64 emulation, if needed
	-docker run --privileged --rm tonistiigi/binfmt --install all
#	remove a maybe outdated build container
	-docker image rm cross-compile-tinamp
#	create a new build container
	docker buildx build --platform linux/arm64 -t cross-compile-tinamp .
#	start the arm64 build
	docker run --user 1000:1000 --platform linux/arm64 --rm -v $(shell pwd):/build -w /build cross-compile-tinamp make aarch64_compile
#       device specific auto scp copy functions for local development
#	k36,arkos clone
#	scp tinamp_aarch64 ark@172.20.1.175:/roms/ports/tinamp/
#	powkiddy v10,rocknix
#	scp tinamp_aarch64 root@172.20.1.175:/roms/ports/tinamp/
#	rg40xx,knulli
#	scp tinamp_aarch64 root@172.20.1.150:/roms/ports/tinamp/
#	rg40xx,muos
#	scp tinamp_aarch64 root@172.20.1.177:/mnt/mmc/ports/tinamp/
#	rgb30, rocknix
#	scp tinamp_aarch64 root@172.20.1.113:/roms/ports/tinamp/
#	rgb20s,arkos+amberlec
#	scp tinamp_aarch64 root@172.20.1.175:/

aarch64_compile: $(OBJS_aarch64)
	$(CC_aarch64) -o $(BUILD_DIR)/$(TARGET_aarch64) $^ $(CFLAGS) $(LDFLAGS)

$(TARGET_amd64): $(OBJS_amd64)
	$(CC_amd64) -o $(BUILD_DIR)/$@ $^ $(CFLAGS) $(LDFLAGS)

$(BUILD_DIR)/%.aarch64.o: %.c
	$(CC_aarch64) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/%.amd64.o: %.c | $(BUILD_DIR)
	$(CC_amd64) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(OBJS_aarch64) $(OBJS_amd64) $(TARGET_aarch64) $(TARGET_amd64)
	-rm $(BUILD_DIR)/$(TARGET_aarch64)
	-rm $(BUILD_DIR)/$(TARGET_amd64)

cleanAll:
	-rm -r .build/
