# Features
 * support for .m3u play lists with UTF-8 support
 * implement presorted.txt support with UTF-8 support
 * support for audio book folders with sub folders e.g. title/volume1/..., title/volume2/...
 * allow the auto save interval to be adjusted and turned off
 * allow screen font color change
 * allow to disable device shutdown
 * explicit support for suspend and resume
 * allow seek while playing

# Code
 * remove all not needed ESP32 code references and unused functions
 * get rid of unused screens
 * make tinamp.c smaller
 * more/better logging
 * also be able to set the system/alsa sound to 100%, support headphone detection?
 * check how to avoid audible clicks from speaker on some models when powering down
