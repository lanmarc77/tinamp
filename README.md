# tinamp
**T**his **I**s **N**ot **A** **M**usic **P**layer audio book player for retro handhelds

# Directory structure
TODO

# Source code structure
TODO

# Compilation
Docker is needed to cross compile for aarch64 architecture. 
Currently Debian Bullseye is used for compilation.  
  
1. Build ffmpeg  
make libffmpeg_aarch64
  
2. Build VLC  
make libvlc_aarch64  
  
3. Build application  
make tinamp_aarch64 (or tinamp_amd64)
  
4. Portmaster package bundle  
make portmaster  
package tinamp.zip is in .build directory  

