# Datei: Dockerfile
FROM debian:bullseye

#FROM ubuntu:19.10
#RUN echo "deb http://old-releases.ubuntu.com/ubuntu/ eoan main restricted universe multiverse" > /etc/apt/sources.list
#RUN echo "deb http://old-releases.ubuntu.com/ubuntu/ eoan-updates main restricted universe multiverse" >> /etc/apt/sources.list
#RUN echo "deb http://old-releases.ubuntu.com/ubuntu/ eoan-security main restricted universe multiverse" >> /etc/apt/sources.list

#patch old debian 11 bullseye repos to avoid bug: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1147093
RUN printf '%s\n' 'deb [check-valid-until=no] http://archive.debian.org/debian bullseye main contrib non-free' 'deb [check-valid-until=no] http://archive.debian.org/debian bullseye-updates main contrib non-free' 'deb [check-valid-until=no] http://snapshot.debian.org/archive/debian-security/20260903T220410Z bullseye-security main contrib non-free' > /etc/apt/sources.list

RUN apt-get update && apt-get install --no-install-recommends -y  gcc wget libsdl2-dev libsdl2-2.0-0 libsdl2-ttf-dev libsdl2-ttf-2.0-0 libsdl2-image-dev libsdl2-image-2.0-0 file pkg-config make git flex gettext libtool build-essential autoconf autopoint yasm checkinstall bison automake libxml2-dev libvlc-dev libopencore-amrwb-dev

WORKDIR /build

