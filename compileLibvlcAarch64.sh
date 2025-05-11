#!/bin/bash
cd .build

cd FFmpeg
make install
ret=$?
if [ "$ret" != "0" ]; then
    echo "make install for ffmpeg did not work"
    exit 2
fi
cd ..

cd vlc
#git checkout 9c47682
mkdir copy_libs
mkdir copy_modules

rm  copy_libs/*
rm  copy_modules/*
echo "Cleaning..."
make clean
echo "Bootstrapping..."
./bootstrap
ret=$?
if [ "$ret" != "0" ]; then
    echo "bootstrap did not work"
    exit 2
fi

echo "Configuring..."
./configure \
  --enable-static \
  --disable-faad \
  --disable-aom \
  --disable-twolame \
  --disable-a52 \
  --disable-flac \
  --disable-vorbis \
  --disable-tremor \
  --disable-speex \
  --disable-opus \
  --disable-theora \
  --disable-matroska \
  --disable-ogg \
  --disable-libva \
  --enable-avcodec \
  --enable-avformat \
  --disable-mpg123 \
  --disable-libxml2 \
  --disable-nls --disable-dbus --disable-gprof --disable-cprof --disable-rpath --disable-debug --disable-coverage --disable-lua --disable-taglib --disable-live555 --disable-dvdread --disable-dvdnav --disable-opencv --disable-sftp --disable-vcd --disable-libcddb --disable-dvbpsi --disable-screen --disable-ogg --disable-mod --disable-wma-fixed --disable-omxil --disable-mad --disable-merge-ffmpeg --disable-faad --disable-flac --disable-realrtsp --disable-vorbis --disable-tremor --disable-speex --disable-theora --disable-schroedinger --disable-fluidsynth --disable-libass --disable-libva --without-x --disable-xcb --disable-xvideo --disable-sdl-image --disable-freetype --disable-fribidi --disable-fontconfig --disable-svg --disable-directx --disable-oss --disable-upnp --disable-skins2 --disable-macosx --disable-ncurses --disable-lirc --disable-libgcrypt --disable-update-check --disable-bluray --disable-samplerate --disable-sid --disable-dxva2 --disable-dav1d --disable-qt --disable-x26410b --disable-a52 --disable-addonmanagermodules --disable-aom --disable-aribb25 --disable-aribsub --disable-asdcp --disable-bpg --disable-caca --disable-chromaprint --disable-chromecast --disable-crystalhd --disable-dc1394 --disable-dca --disable-decklink --disable-dsm --disable-dv1394 --disable-fluidlite --disable-gme --disable-goom --disable-jack --disable-kai --disable-kate --disable-kva --disable-libplacebo --disable-linsys --disable-mfx --disable-microdns --disable-mmal --disable-mtp --disable-notify --disable-projectm --disable-shine --disable-shout --disable-sndio --disable-spatialaudio --disable-srt --disable-telx --disable-tiger --disable-twolame --disable-vdpau --disable-vsxu --disable-wasapi --disable-x262 --disable-zvbi \
  --disable-fdkaac \
  --disable-archive \
  --disable-live555 \
  --disable-dc1394 \
  --disable-dv1394 \
  --disable-linsys \
  --disable-dvdread \
  --disable-dvdnav \
  --disable-bluray \
  --disable-opencv \
  --disable-smbclient \
  --disable-dsm \
  --disable-sftp \
  --disable-nfs \
  --disable-smb2 \
  --disable-v4l2 \
  --disable-amf-scaler \
  --disable-amf-enhancer \
  --disable-amf-frc \
  --disable-decklink \
  --disable-vcd \
  --disable-libcddb \
  --disable-screen \
  --disable-vnc \
  --disable-freerdp \
  --disable-realrtsp \
  --disable-qt \
  --disable-skins2 \
  --disable-mad \
  --disable-dbus \
  --disable-lua \
  --disable-nls \
  --disable-mmx \
  --disable-sse \
  --disable-swscale \
  --disable-dvdread \
  --disable-dvdnav \
  --disable-vcd \
  --disable-v4l2 \
  --disable-xcb \
  --disable-xvideo \
  --disable-ncurses \
  --disable-lirc \
  --disable-srt \
  --disable-goom \
  --disable-projectm \
  --disable-vsxu \
  --disable-avahi \
  --disable-udev \
  --disable-sout \
  --enable-alsa \
  --disable-pulse \
  --disable-sid \
  --disable-gme \
  --disable-shine \
  --disable-vlc \
  CFLAGS="-Os -fPIC" \
  CXXFLAGS="-Os -fPIC"
ret=$?
if [ "$ret" != "0" ]; then
    echo "configure did not work"
    exit 2
fi

make -j10

#copy libvlc libraries
cp src/.libs/libvlccore.so.9 copy_libs/
cp lib/.libs/libvlc.so.5 copy_libs/
#copy libvlc plugin modules
cp modules/.libs/*.so copy_modules
#get rid of all graphics modules they pull in unnecessarry dependencies
rm copy_modules/libgl*
#find all library dependencies
find "copy_modules/" -type f | while read -r file; do
    echo "[INFO] Processing: $file"
    ldd "$file" 2>/dev/null | grep '=>' | awk '{print $3}' | while read -r lib; do
        if [[ -f "$lib" ]]; then
            cp -u "$lib" "copy_libs/"
            echo "  [LIB] Copied: $lib"
        fi
    done
    echo
done

#use the standard system libraries of the devices instead of the ones from the compile container
rm copy_libs/libasound*
rm copy_libs/libm.so.6
rm copy_libs/libc.so.6
rm copy_libs/libpthread.so.0
rm copy_libs/librt.so.1
rm copy_libs/libdl*
rm copy_libs/libgcc*
rm copy_libs/libstdc++*

#prepare a ready to copy libs folder
cd ..
mkdir -p libs/vlc
chown -R 1000:1000 vlc

rm libs/*
rm libs/vlc/*
cp vlc/copy_libs/* libs/
cp vlc/copy_modules/* libs/vlc/
chown -R 1000:1000 libs

exit

