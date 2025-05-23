/*  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef FORMAT_HELPER_C
#define FORMAT_HELPER_C
#include <string.h>
#include "format_helper.h"

/**
  * @brief  calculates a byte based file position (to seek to) from a given play position,
  *         filename (extension to be precise) and file format specific data
  *
  * @param currentPlaySecond current second of play position
  * @param currentPlayMinute current minute of play position
  * @param fileName pointer to filename to extract the extension from
  * @param currentPlaySize the size of the file
  * @param currentPlayOffset the offset used by this files format (e.g. from header)
  * @param currentPlayBlockSize the size of one block/frame of this files format
  * @param currentPlayBitrate the bitrate of the file
  * @param channels the number of channels of the file
  * 
  * @return calculated file byte position, in cases of errors 0 so the file would play from the start
  */
uint64_t FORMAT_HELPER_getFilePosByPlayTimeAndExtension(uint8_t currentPlaySecond,uint16_t currentPlayMinute,char* fileName,uint64_t currentPlaySize,uint64_t currentPlayOffset,uint64_t currentPlayBlockSize,uint64_t currentPlayBitrate,int32_t channels){
    uint8_t fileFormat=FORMAT_HELPER_getFileType(fileName);
    uint64_t playPos=0;
    if(fileFormat==FORMAT_HELPER_FILE_TYPE_AMR){
        playPos=(((currentPlaySecond+currentPlayMinute*60)*50)*currentPlayBlockSize)+currentPlayOffset;
    }else if(fileFormat==FORMAT_HELPER_FILE_TYPE_MP3){
        playPos=(((currentPlaySecond+currentPlayMinute*60)*1000)*currentPlayBitrate)/8+currentPlayOffset;
    }else if(fileFormat==FORMAT_HELPER_FILE_TYPE_OGG){
        playPos=((currentPlaySecond+currentPlayMinute*60)*currentPlayBlockSize)+currentPlayOffset;
    }else{
        playPos=(((currentPlaySecond+currentPlayMinute*60)*1000)*currentPlayBitrate)/8+currentPlayOffset;
    }
    if(playPos>=currentPlaySize){
        playPos=0;
    }
    return playPos;
}


/**
  * @brief  calculates the current play position based on a given file position
  *         based on the given filenames extension and file format specific data
  *
  * @param[out] currentPlaySecond pointer to store the calculated second of play position
  * @param[out] currentPlayMinute pointer to store the calculated minute of play position
  * @param[out] percent pointer to store the played percent by time of the whole file
  * @param[in] fileName pointer to filename to extract the extension from
  * @param[in] currentPlayPosition the current file/play position in the played file
  * @param[out] currentPlaySize the size of the file
  * @param currentPlayOffset the offset used by this files format (e.g. from header)
  * @param currentPlayBlockSize the size of one block/frame of this files format
  * @param currentPlayBitrate the bitrate of the file
  * @param[out] allPlaySecond pointer to store the calculated number of seconds of the whole file (playlengh)
  * @param[out] allPlayMinute pointer to store the calculated minute of minutes of the whole file (playlengh)
  * @param channels the number of channels of the file
  * 
  */
void FORMAT_HELPER_getPlayTimeByExtension(uint8_t* currentPlaySecond,uint16_t* currentPlayMinute,uint8_t* percent, char* fileName,uint64_t currentPlayPosition,uint64_t currentPlaySize,uint64_t currentPlayOffset,uint64_t currentPlayBlockSize,uint64_t currentPlayBitrate,uint8_t* allPlaySecond,uint16_t* allPlayMinute,int32_t channels){
    uint8_t fileFormat=FORMAT_HELPER_getFileType(fileName);
    if(strlen(fileName)<3){
        *percent=0;
        *currentPlayMinute=0;
        *currentPlaySecond=0;
    }
    if(currentPlayOffset>currentPlayPosition) currentPlayPosition=currentPlayOffset;
    if(((*currentPlayMinute==0)&&(*currentPlaySecond==0))||(currentPlayPosition==0)){
        *percent=0;
    }
    if(fileFormat==FORMAT_HELPER_FILE_TYPE_AMR){
        if(currentPlaySize!=0){
            *percent=(currentPlayPosition*100)/currentPlaySize;
        }
        if(currentPlayBlockSize!=0){
            *currentPlayMinute=(((currentPlayPosition-currentPlayOffset)/currentPlayBlockSize)/50)/60;
            *currentPlaySecond=(((currentPlayPosition-currentPlayOffset)/currentPlayBlockSize)/50)%60;
            if(allPlayMinute!=NULL){
                *allPlayMinute=(((currentPlaySize-currentPlayOffset)/currentPlayBlockSize)/50)/60;
            }
            if(allPlaySecond!=NULL){
                *allPlaySecond=(((currentPlaySize-currentPlayOffset)/currentPlayBlockSize)/50)%60;
            }
        }
    }else if(fileFormat==FORMAT_HELPER_FILE_TYPE_MP3){
        if(currentPlaySize!=0){
            *percent=(currentPlayPosition*100)/currentPlaySize;
        }
        if(currentPlayBitrate!=0){
            *currentPlayMinute=((8*(currentPlayPosition-currentPlayOffset)/currentPlayBitrate)/1000)/60;
            *currentPlaySecond=((8*(currentPlayPosition-currentPlayOffset)/currentPlayBitrate)/1000)%60;
            if(allPlayMinute!=NULL){
                *allPlayMinute=((8*(currentPlaySize-currentPlayOffset)/currentPlayBitrate)/1000)/60;
            }
            if(allPlaySecond!=NULL){
                *allPlaySecond=((8*(currentPlaySize-currentPlayOffset)/currentPlayBitrate)/1000)%60;
            }
        }
    }else if(fileFormat==FORMAT_HELPER_FILE_TYPE_OGG){
        if(currentPlaySize!=0){
            *percent=(currentPlayPosition*100)/currentPlaySize;
        }
        if(currentPlayBlockSize!=0){
            *currentPlayMinute=((currentPlayPosition-currentPlayOffset)/(currentPlayBlockSize))/60;
            *currentPlaySecond=((currentPlayPosition-currentPlayOffset)/(currentPlayBlockSize))%60;
            if(allPlayMinute!=NULL){
                *allPlayMinute=((currentPlaySize-currentPlayOffset)/(currentPlayBlockSize))/60;
            }
            if(allPlaySecond!=NULL){
                *allPlaySecond=((currentPlaySize-currentPlayOffset)/(currentPlayBlockSize))%60;
            }
        }
    }else{
        if(currentPlaySize!=0){
            *percent=(currentPlayPosition*100)/currentPlaySize;
        }
        if(currentPlayBitrate!=0){
            *currentPlayMinute=((8*(currentPlayPosition-currentPlayOffset)/currentPlayBitrate)/1000)/60;
            *currentPlaySecond=((8*(currentPlayPosition-currentPlayOffset)/currentPlayBitrate)/1000)%60;
            if(allPlayMinute!=NULL){
                *allPlayMinute=((8*(currentPlaySize-currentPlayOffset)/currentPlayBitrate)/1000)/60;
            }
            if(allPlaySecond!=NULL){
                *allPlaySecond=((8*(currentPlaySize-currentPlayOffset)/currentPlayBitrate)/1000)%60;
            }
        }
    }
}

uint8_t FORMAT_HELPER_getFileType(char* fileName){
    uint16_t length=strlen(fileName);
    if(length<3){
        return FORMAT_HELPER_FILE_TYPE_UNKNOWN;
    }
    if(strncasecmp(&fileName[length-3],"mp3",3)==0){
        return FORMAT_HELPER_FILE_TYPE_MP3;
    }else if((strncasecmp(&fileName[length-3],"mp4",3)==0)||(strncasecmp(&fileName[length-3],"m4a",3)==0)||(strncasecmp(&fileName[length-3],"aac",3)==0)||(strncasecmp(&fileName[length-3],"m4b",3)==0)){
        return FORMAT_HELPER_FILE_TYPE_M4A;
    }else if((strncasecmp(&fileName[length-3],"awb",3)==0)||(strncasecmp(&fileName[length-3],"amr",3)==0)){
        return FORMAT_HELPER_FILE_TYPE_AMR;
    }else if(strncasecmp(&fileName[length-3],"ogg",3)==0){
        return FORMAT_HELPER_FILE_TYPE_OGG;
    }
    return FORMAT_HELPER_FILE_TYPE_UNKNOWN;
}

//from: http://www.mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm
uint16_t SD_PLAY_mp3BitratesV1[16]={0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
uint32_t SD_PLAY_mp3SampleRatesV1[4]={8*441,8*480,8*320,8*0};
uint16_t SD_PLAY_mp3BitratesV2[16]={0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0};
uint32_t SD_PLAY_mp3SampleRatesV2[4]={22050,24000,16000,0};
uint8_t SD_PLAY_mp3Channels[4]={2,2,2,1};
/**
  * @brief  gets MP3 specific format information of a file
  *
  * @param[in] file handle to a MP3 file to check
  * @param[out] sampleRate pointer where the sample rate of the file will be stored
  * @param[out] bits pointer where the resolution in bits of a sample will be stored
  * @param[out] channels pointer where the number of channels will be stored
  * @param[out] offset pointer where the offset (e.g. from a fixed header) will be stored
  * @param[out] bitrate pointer where the bitrate of the file be stored
  * @param[out] blockSize pointer where the blocksize (also framesize) will be stored
  * @param[in] fileSize the size of the file
  * 
  * @return 0=ok, 1=error
  */
uint8_t FORMAT_HELPER_getMP3FormatInformation(FILE* file,int32_t* sampleRate, int32_t* bits, int32_t* channels,uint64_t* offset,uint64_t* bitrate,uint64_t* blockSize,uint64_t fileSize){
    char buf[16];
    uint8_t ret=1;
    uint8_t pad=0;
    uint8_t readFrames=2;//read two mpeg frames to jump over a possible info frame that might have wrong stream information
    uint8_t samplesPerFrame=144;
    fseek(file,0,SEEK_SET);
    int64_t read=fread(&buf[0],1,sizeof(buf),file);
    if(read == sizeof(buf)){//id3v2 tag + full header?
        if((buf[0]=='I')&&(buf[1]=='D')&&(buf[2]=='3')){//skip id3v2 header
            *offset=buf[6]*128*128*128+buf[7]*128*128+buf[8]*128+buf[9]+10;
            fseek(file,*offset,SEEK_SET);
            read=fread(&buf[0],1,sizeof(buf),file); //"first" (re)read
        }else{
            *offset=0;
        }
        while(readFrames--){
            if((buf[0]==0xFF)&&((buf[1]&0xF0)==0xF0)){//because we use 0xF0 here we ignore/filter out MPEG2.5
                //if we are here we should be at a data frame start
                *bits=16;//esp decoder always delivers 16bit
                *channels=SD_PLAY_mp3Channels[(buf[3]>>6)&0x03];
                if(buf[2]&0x02){
                    pad=1;
                }else{
                    pad=0;
                }
                if(((buf[1]>>3)&0x03)==0x03){//MPEG1
                    samplesPerFrame=144;
                    //ESP_LOGI("MP3","V1 Layer 3 %x %x %x %llu",buf[1], buf[2], buf[3],*offset);
                    *sampleRate=((SD_PLAY_mp3SampleRatesV1[(buf[2]>>2)&0x03])/8)*100;
                    *bitrate=SD_PLAY_mp3BitratesV1[(buf[2]>>4)];
                    if((*sampleRate!=0)&&(*bitrate!=0)){//avoid division by 0 if frame was wrongfully detected
                        *blockSize=1+(10L*(*bitrate)* 1152L) / SD_PLAY_mp3SampleRatesV1[(buf[2]>>2)&0x03];
                    }
                }else{//MPEG2
                    samplesPerFrame=72;
                    //ESP_LOGI("MP3","V2 Layer 3 %x %x %x %llu",buf[1], buf[2], buf[3],*offset);
                    *sampleRate=SD_PLAY_mp3SampleRatesV2[(buf[2]>>2)&0x03];
                    *bitrate=SD_PLAY_mp3BitratesV2[(buf[2]>>4)];
                    if((*sampleRate!=0)&&(*bitrate!=0)){//avoid division by 0 if frame was wrongfully detected
                        *blockSize=1+((10L*(*bitrate)* 576L) / SD_PLAY_mp3SampleRatesV2[(buf[2]>>2)&0x03])/8;
                    }
                }
                if((*sampleRate!=0)&&(*bitrate!=0)){//avoid division by 0 if frame was wrongfully detected
                    //next frame
                    *offset+=((samplesPerFrame* (*bitrate*1000)) / *sampleRate)+pad;
                    //ESP_LOGI("MP3","New offset %llu",*offset);
                    fseek(file,*offset,SEEK_SET);
                    read=fread(&buf[0],1,sizeof(buf),file);
                    ret=0;
                }else{
                    ret=1;
                }
            }
        }
    }
    fseek(file,0,SEEK_SET);
    return ret;
}

/**
  * @brief  gets OGG specific format information of a file
  *
  * @param[in] file handle to a OGG file to check
  * @param[out] sampleRate pointer where the sample rate of the file will be stored
  * @param[out] bits pointer where the resolution in bits of a sample will be stored
  * @param[out] channels pointer where the number of channels will be stored
  * @param[out] offset pointer where the offset (e.g. from a fixed header) will be stored
  * @param[out] bitrate pointer where the bitrate of the file be stored
  * @param[out] blockSize pointer where the blocksize (also framesize) will be stored
  * @param[in] fileSize the size of the file
  * 
  * @return 0=ok, 1=error
  */
uint8_t FORMAT_HELPER_getOGGFormatInformation(FILE* file,int32_t* sampleRate, int32_t* bits, int32_t* channels,uint64_t* offset,uint64_t* bitrate,uint64_t* blockSize,uint64_t fileSize){
    char buf[60];
    uint8_t ret=1;
    fseek(file,0,SEEK_SET);
    int64_t read=fread(&buf[0],1,sizeof(buf),file);
    if(read==sizeof(buf)){
        *offset=29;
        *blockSize=0;
        *bitrate=0;
        if((buf[29]=='v')&&(buf[30]=='o')&&(buf[31]=='r')&&(buf[32]=='b')&&(buf[33]=='i')&&(buf[34]=='s')){//vorbis header?
            *channels=buf[39];
            *sampleRate=(buf[40]+buf[41]*256+buf[42]*256*256+buf[43]*256*256*256);
            *bitrate=(buf[48]+buf[49]*256+buf[50]*256*256+buf[51]*256*256*256)/1000;
            *bits=16;//esp decoder always delivers 16bit
            fseek(file,-29,SEEK_END);
            read=fread(buf,1,29,file); //read the final page
            if((buf[0]=='O')&&(buf[1]=='g')&&(buf[2]=='g')&&(buf[3]=='S')){//final page detected ok
                uint64_t numberOfSamples=(buf[6]+buf[7]*256+buf[8]*256*256+buf[9]*256*256*256);
                *blockSize=(fileSize*100)/(100*numberOfSamples/ *sampleRate);
            }else{//if no final page with number of samples is available, make a guess, at least the file can be played and seeked then, even if the time code is wrong
                *blockSize=3072;
            }
            ret=0;
        }
    }
    fseek(file,0,SEEK_SET);
    return ret;
}

uint16_t SD_PLAY_amrNBitrates[16]={4750,5150,5900,6700,7400,7950,10200,12200,0,0,0,0,0,0,0,0};
uint16_t SD_PLAY_amrWBitrates[16]={6600,8850,12650,14250,15850,18250,19850,23050,23850,0,0,0,0,0,0,0};
/**
  * @brief  gets AMR specific format information of a file
  *
  * @param[in] file handle to a AMR file to check
  * @param[out] sampleRate pointer where the sample rate of the file will be stored
  * @param[out] bits pointer where the resolution in bits of a sample will be stored
  * @param[out] channels pointer where the number of channels will be stored
  * @param[out] offset pointer where the offset (e.g. from a fixed header) will be stored
  * @param[out] bitrate pointer where the bitrate of the file be stored
  * @param[out] blockSize pointer where the blocksize (also framesize) will be stored
  * @param[in] fileSize the size of the file
  * @param[out] upMix pointer for a boolean which determines if single channel audio should upmixed
  *                   to stereo so that left and right are identical, some DACs need this or they play
  *                   audio only on one side of the headphone
  * 
  * @return 0=ok, 1=error
  */
 uint8_t FORMAT_HELPER_getAMRFormatInformation(FILE* file,int32_t* sampleRate, int32_t* bits, int32_t* channels,uint64_t* offset,uint64_t* bitrate,uint64_t* blockSize,uint64_t fileSize,bool* upMix){
    char buf[16];
    uint8_t ret=1;
    fseek(file,0,SEEK_SET);
    int64_t read=fread(&buf[0],1,sizeof(buf),file);
    if(read==sizeof(buf)){
        if(strncmp(&buf[0],"#!AMR-WB\n",9)==0){
            *channels=2;
            *upMix=true;
            *sampleRate=16000;
            *bits=16;//esp decoder always delivers 16bit
            *bitrate=SD_PLAY_amrWBitrates[(buf[9]>>3)&0x0F];
            *blockSize=(*bitrate/(50*8))+2;
            *offset=9;
            ret=0;
        }else if(strncmp(&buf[0],"#!AMR\n",6)==0){
            *channels=2;
            *upMix=true;
            *sampleRate=8000;
            *bits=16;//esp decoder always delivers 16bit
            *bitrate=SD_PLAY_amrNBitrates[(buf[6]>>3)&0x0F];
            *blockSize=(*bitrate/(50*8))+2;
            *offset=6;
            ret=0;
        }
    }
    fseek(file,0,SEEK_SET);
    return ret;
}

/**
  * @brief  gets M4A specific format information of a file, currently unused as mpeg4 audio
  *         is pretty buggy on esp-adf
  *
  * @param[in] file handle to a M4A file to check
  * @param[out] sampleRate pointer where the sample rate of the file will be stored
  * @param[out] bits pointer where the resolution in bits of a sample will be stored
  * @param[out] channels pointer where the number of channels will be stored
  * @param[out] offset pointer where the offset (e.g. from a fixed header) will be stored
  * @param[out] bitrate pointer where the bitrate of the file be stored
  * @param[out] blockSize pointer where the blocksize (also framesize) will be stored
  * @param[in] fileSize the size of the file
  * 
  * @return 0=ok, 1=error
  */
uint8_t FORMAT_HELPER_getM4AFormatInformation(FILE* file,int32_t* sampleRate, int32_t* bits, int32_t* channels,uint64_t* offset,uint64_t* bitrate,uint64_t* blockSize,uint64_t fileSize){
    *sampleRate=44100;
    *bits=16;
    *channels=2;
    *offset=0;
    *bitrate=128*1024;
    *blockSize=64;
    return 1;
}

#endif
