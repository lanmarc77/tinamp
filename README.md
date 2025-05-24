# tinamp
**T**his **I**s **N**ot **A** **M**usic **P**layer audio book player for retro handhelds

# Directory structure
TODO

# Source code structure
TODO

# Compilation
Compilation needs docker installed.  
Debian 11 Bullseye arm64 is used for compiling to support older libc operating systems.  
  
1. Build ffmpeg  
```make libffmpeg_aarch64```
  
2. Build VLC, also collects libraries for later packaging  
```make libvlc_aarch64```
  
3. Build application  
```make tinamp_aarch64``` (or tinamp_amd64 for local development)
  
4. Portmaster package bundle  
```make portmaster``` 
package tinamp.zip is in .build directory as distributable portmaster package  

