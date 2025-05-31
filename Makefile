CC_aarch64 = gcc
CC_armhf = gcc
CC_amd64 = gcc
LDFLAGS = -lSDL2 -lSDL2_ttf -lvlc -Wl,--strip-debug
CFLAGS = -Wall -g

SRCS = md5.c saves.c format_helper.c ff_handling.c sd_play.c queue.c util.c battery.c display.c input.c SDL_FontCache.c ui_elements.c screens.c tinamp.c

BUILD_DIR = .build
TARGET_aarch64 = tinamp_aarch64
TARGET_armhf = tinamp_armhf
TARGET_amd64 = tinamp_amd64

OBJS_aarch64 = $(patsubst %.c, $(BUILD_DIR)/%.aarch64.o, $(SRCS))
OBJS_armhf = $(patsubst %.c, $(BUILD_DIR)/%.armhf.o, $(SRCS))
OBJS_amd64 = $(patsubst %.c, $(BUILD_DIR)/%.amd64.o, $(SRCS))

all: $(TARGET_amd64)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


portmaster:
	-rm $(BUILD_DIR)/tinamp.zip
	-rm -r $(BUILD_DIR)/tinamp
	./buildPortmasterPackage.sh

libvlc_aarch64_compile:
	cd $(BUILD_DIR) && tar -xzf ../vlc.tar.gz && mv vlc-* vlc
	cd $(BUILD_DIR)/vlc/src && echo "9c4768291ee0ce8e29fdadf3e05cbde2714bbe0c" > revision.txt
	./compileLibvlc.sh

libvlc_armhf_compile:
	cd $(BUILD_DIR) && tar -xzf ../vlc.tar.gz && mv vlc-* vlc
	cd $(BUILD_DIR)/vlc/src && echo "9c4768291ee0ce8e29fdadf3e05cbde2714bbe0c" > revision.txt
	./compileLibvlc.sh

libffmpeg_aarch64_compile:
	cd $(BUILD_DIR) && tar -xzf ../FFmpeg.tar.gz && mv FFmpeg-* FFmpeg
	./compileFFmpeg.sh

libffmpeg_armhf_compile:
	cd $(BUILD_DIR) && tar -xzf ../FFmpeg.tar.gz && mv FFmpeg-* FFmpeg
	./compileFFmpeg.sh

libvlc_aarch64: $(BUILD_DIR)
	-docker run --privileged --rm tonistiigi/binfmt --install all
	-docker image rm cross-compile-tinamp
	docker buildx build --platform linux/arm64 -t cross-compile-tinamp .
	docker run --platform linux/arm64 --rm -v $(shell pwd):/build -w /build cross-compile-tinamp make libvlc_aarch64_compile

libvlc_armhf: $(BUILD_DIR)
	-docker run --privileged --rm tonistiigi/binfmt --install all
	-docker image rm cross-compile-tinamp-armhf
	docker buildx build --platform linux/armhf -t cross-compile-tinamp-armhf .
	docker run --platform linux/armhf --rm -v $(shell pwd):/build -w /build cross-compile-tinamp-armhf make libvlc_armhf_compile

libffmpeg_aarch64: $(BUILD_DIR)
	-docker run --privileged --rm tonistiigi/binfmt --install all
	-docker image rm cross-compile-tinamp
	docker buildx build --platform linux/arm64 -t cross-compile-tinamp .
	docker run --platform linux/arm64 --rm -v $(shell pwd):/build -w /build cross-compile-tinamp make libffmpeg_aarch64_compile

libffmpeg_armhf: $(BUILD_DIR)
	-docker run --privileged --rm tonistiigi/binfmt --install all
	-docker image rm cross-compile-tinamp-armhf
	docker buildx build --platform linux/armhf -t cross-compile-tinamp-armhf .
	docker run --platform linux/armhf --rm -v $(shell pwd):/build -w /build cross-compile-tinamp-armhf make libffmpeg_armhf_compile

$(TARGET_aarch64): $(BUILD_DIR)
	-docker run --privileged --rm tonistiigi/binfmt --install all
	-docker image rm cross-compile-tinamp
	docker buildx build --platform linux/arm64 -t cross-compile-tinamp .
	docker run --user 1000:1000 --platform linux/arm64 --rm -v $(shell pwd):/build -w /build cross-compile-tinamp make aarch64_compile

$(TARGET_armhf): $(BUILD_DIR)
	-docker run --privileged --rm tonistiigi/binfmt --install all
	-docker image rm cross-compile-tinamp-armhf
	docker buildx build --platform linux/armhf -t cross-compile-tinamp-armhf .
	docker run --user 1000:1000 --platform linux/armhf --rm -v $(shell pwd):/build -w /build cross-compile-tinamp-armhf make armhf_compile

aarch64_compile: $(OBJS_aarch64)
	$(CC_aarch64) -o $(BUILD_DIR)/$(TARGET_aarch64) $^ $(CFLAGS) $(LDFLAGS)

armhf_compile: $(OBJS_armhf)
	$(CC_armhf) -o $(BUILD_DIR)/$(TARGET_armhf) $^ $(CFLAGS) $(LDFLAGS)

$(TARGET_amd64): $(OBJS_amd64)
	$(CC_amd64) -o $(BUILD_DIR)/$@ $^ $(CFLAGS) $(LDFLAGS)

$(BUILD_DIR)/%.aarch64.o: %.c
	$(CC_aarch64) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/%.armhf.o: %.c
	$(CC_armhf) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/%.amd64.o: %.c | $(BUILD_DIR)
	$(CC_amd64) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(OBJS_aarch64) $(OBJS_armhf) $(OBJS_amd64) $(TARGET_aarch64) $(TARGET_armhf) $(TARGET_amd64)
	-rm $(BUILD_DIR)/$(TARGET_aarch64)
	-rm $(BUILD_DIR)/$(TARGET_armhf)
	-rm $(BUILD_DIR)/$(TARGET_amd64)

cleanAll:
	-rm -r .build/
