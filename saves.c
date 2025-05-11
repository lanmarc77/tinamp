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
#ifndef SAVES_C
#define SAVES_C
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include "md5.h"
#include "saves.h"

char SAVES_baseFolder[FF_FILE_PATH_MAX]={0};
char SAVES_settingsFileName[]="settings.cfg";
/**
  * @brief  mounts the vfs spiffs partition and formats it if needed
  */
void SAVES_init(char *baseFolder){
    strcpy(&SAVES_baseFolder[0],baseFolder);
    strcat(&SAVES_baseFolder[0],"saves/");
}

/**
  * @brief  loads the stored player settings
  *
  * @param[out] settings pointer to player settings structure where the loaded settings will be stored
  * 
  * @return 0=ok, settings loaded, 1=error, no settings loaded
  */
uint8_t SAVES_loadSettings(SAVES_settings_t *settings){
    char settingsFileName[FF_FILE_PATH_MAX+128];
    sprintf(&settingsFileName[0],"%s%s",&SAVES_baseFolder[0],&SAVES_settingsFileName[0]);
    FILE* f = fopen(&settingsFileName[0], "rb");
    if (f == NULL) {
        return 1;
    }
    int32_t result=fread(settings,1,sizeof(SAVES_settings_t),f);
    if ((result<sizeof(SAVES_settings_t)&&(feof(f)))
        ||(result==sizeof(SAVES_settings_t)))
    {
        fclose(f);
        return 0;
    }else{
//        ESP_LOGE(SAVES_LOG_TAG,"Settings restore error. Result %li",result);
    }
    fclose(f);
    return 1;
}

/**
  * @brief  saves the current player settings
  *
  * @param[in] settings pointer to player settings structure with filled player settings
  * 
  * @return 0=ok, settings stored, 1=error, settings not stored
  */
uint8_t SAVES_saveSettings(SAVES_settings_t *settings){
    char tempSettingsFileName[FF_FILE_PATH_MAX+128];
    char newSettingsFileName[FF_FILE_PATH_MAX+128];
    sprintf(&tempSettingsFileName[0],"%stmp_%s",&SAVES_baseFolder[0],&SAVES_settingsFileName[0]);
    sprintf(&newSettingsFileName[0],"%s%s",&SAVES_baseFolder[0],&SAVES_settingsFileName[0]);
    FILE* f = fopen(&tempSettingsFileName[0], "wb");
    settings->version=0;
    if (f == NULL) {
//        ESP_LOGE(SAVES_LOG_TAG,"Could not open settings.cfg for writing.");
        return 1;
    }
    int32_t result=fwrite(settings,1,sizeof(SAVES_settings_t),f);
    if(result==sizeof(SAVES_settings_t)){
        fclose(f);
        rename(&tempSettingsFileName[0],&newSettingsFileName[0]);
        return 0;
    }else{
//        ESP_LOGE(SAVES_LOG_TAG,"Settings save error. Result %li",result);
    }
    fclose(f);
    return 1;
}

/**
  * @brief returns a bookmarkFileName=bookmarkID from a audio book folderName
  *
  * @param[in] folderName pointer to an audio book folderName
  * @param[out] saveFileName pointer to string of the name of the bookmark file in SPIFFS
  * 
  */
void SAVES_getBookmarkFileFromFolderName(char* folderName,char* saveFileName){
    unsigned char md5Hash[16];
    md5_context ctx;
    md5_init(&ctx);
    md5_digest(&ctx, folderName, strlen(folderName));
    md5_output(&ctx, &md5Hash[0]);
    sprintf(saveFileName,"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X.b",md5Hash[0],md5Hash[1],md5Hash[2],md5Hash[3],md5Hash[4],md5Hash[5],md5Hash[6],md5Hash[7],md5Hash[8],md5Hash[9],md5Hash[10],md5Hash[11],md5Hash[12],md5Hash[13],md5Hash[14],md5Hash[15]);
}

/**
  * @brief checks if bookmark exists for a specific audio book by folderName and if so loads the bookmark
  *
  * @param[in] folderName pointer to an audio book folderName
  * @param[out] save pointer to bookmark structure where the bookmark data will be stored
  * 
  * @return 0=ok, bookmark exists and loaded, 1=error, bookmark does not exist
  */
uint8_t SAVES_existsBookmark(char* folderName,SAVES_saveState_t* save){
    char fileNameOnly[64];
    char saveFileName[FF_FILE_PATH_MAX+128];

    SAVES_getBookmarkFileFromFolderName(folderName,&fileNameOnly[0]);
    sprintf(&saveFileName[0],"%s%s",&SAVES_baseFolder[0],&fileNameOnly[0]);

    //ESP_LOGI(SAVES_LOG_TAG,"Searching for bookmark file %s",&saveFileName[0]);
    FILE* f = fopen(&saveFileName[0], "rb");
    if (f == NULL) {
        return 1;
    }
    int32_t result=fread(save,1,sizeof(SAVES_saveState_t),f);
    if ((result<sizeof(SAVES_saveState_t)&&(feof(f)))
        ||(result==sizeof(SAVES_saveState_t))
    ){
//        ESP_LOGI(SAVES_LOG_TAG,"Restored ok. folder \"%s\" file \"%s\" ",save->folderName,save->fileName);
        fclose(f);
        return 0;
    }else{
//        ESP_LOGE(SAVES_LOG_TAG,"Restore error. Result %li",result);
    }
    fclose(f);
    return 1;
}

/**
  * @brief saves a bookmark exists for a specific audio book specified by folderName
  *
  * @param[in] folderName pointer to an audio book folderName
  * @param[out] save pointer to filled bookmark structure
  * 
  * @return 0=ok, bookmark stored, 1=error, bookmark not stored
  */
uint8_t SAVES_saveBookmark(char* folderName,SAVES_saveState_t* save){
    char saveFileName[64];
    char tempSaveFileName[FF_FILE_PATH_MAX+128];
    char newSaveFileName[FF_FILE_PATH_MAX+128];
    SAVES_getBookmarkFileFromFolderName(folderName,&saveFileName[0]);
    sprintf(&tempSaveFileName[0],"%stmp_%s",&SAVES_baseFolder[0],&saveFileName[0]);
    sprintf(&newSaveFileName[0],"%s%s",&SAVES_baseFolder[0],&saveFileName[0]);

    //ESP_LOGI(SAVES_LOG_TAG,"Saving for bookmark file %s at position %llu for playfile %s",&saveFileName[0],save->playPosition,save->fileName);
    FILE* f = fopen(&tempSaveFileName[0], "wb");
    save->version=0;
    if (f == NULL) {
//        ESP_LOGE(SAVES_LOG_TAG,"Could not open bookmark for writing.");
        return 1;
    }
    strcpy(&save->folderName[0],folderName);
    int32_t result=fwrite(save,1,sizeof(SAVES_saveState_t),f);
    if(result==sizeof(SAVES_saveState_t)){
//        ESP_LOGI(SAVES_LOG_TAG,"Saved ok. Free space: %li",SAVES_getUsedSpaceLevel());
        fclose(f);
        rename(&tempSaveFileName[0],&newSaveFileName[0]);
        return 0;
    }else{
//        ESP_LOGE(SAVES_LOG_TAG,"Save error. Result %li",result);
    }
    fclose(f);
    return 1;
}

/**
  * @brief deletes bookmarks from the spiffs which folders+files do not exist on the sd card (anymore)
  *
  * @param flags 0=delete unused bookmarks, 1=only count number of bookmarks but do not clean up, 15=force delete also used bookmarks
  * 
  * @return >=0 number of files deleted, <0 error occured
  */
int32_t SAVES_cleanOldBookmarks(uint8_t flags,char *audioBookFolder){
    char fullfile[FF_FILE_PATH_MAX];
    char fullfile2[FF_FILE_PATH_MAX];
    int32_t fileCounter=0;
    struct dirent *currentEntry;
    SAVES_saveState_t save;
    FILE* f;
    FILE* f2;
    DIR *dir = opendir(&SAVES_baseFolder[0]);
    if(dir){
        while ((currentEntry = readdir(dir)) != NULL) {
            if(currentEntry->d_type!=DT_DIR){
                if(currentEntry->d_name[strlen(currentEntry->d_name)-1]=='b'){//file extension .b?
                    if(flags==1){
                        fileCounter++;
                    }else{
                        strcpy(&fullfile[0],&SAVES_baseFolder[0]);
                        //strcat(&fullfile[0],"/");
                        strcat(&fullfile[0],currentEntry->d_name);
                        if(flags==15){
                            if(remove(&fullfile[0])==0){
                                fileCounter++;
//                                ESP_LOGI(SAVES_LOG_TAG,"Force deleted save: %s",&fullfile[0]);
                            }else{
//                                ESP_LOGE(SAVES_LOG_TAG,"ERROR force deleting save: %s",&fullfile[0]);
                            }
                        }else{
                            f = fopen(&fullfile[0], "rb");
                            if (f == NULL) {
//                                ESP_LOGE(SAVES_LOG_TAG,"Could not open save file %s from SPIFFS.",&fullfile[0]);
                                continue;//ignore files that cant be opened
                            }
                            int32_t result=fread(&save,1,sizeof(SAVES_saveState_t),f);
                            if(result>=sizeof(SAVES_saveState_t)){
                                strcpy(&fullfile2[0],audioBookFolder);
                                strcat(&fullfile2[0],"/");
                                strcat(&fullfile2[0],&save.folderName[0]);
                                strcat(&fullfile2[0],"/");
                                strcat(&fullfile2[0],&save.fileName[0]);
                                f2 = fopen(&fullfile2[0], "rb");
                                if(f2==NULL){
                                    if(remove(&fullfile[0])==0){
                                        fileCounter++;
//                                        ESP_LOGI(SAVES_LOG_TAG,"Deleted missing save: %s->%s",&fullfile[0],&fullfile2[0]);
                                    }else{
//                                        ESP_LOGE(SAVES_LOG_TAG,"ERROR deleting missing save: %s->%s",&fullfile[0],&fullfile2[0]);
                                    }
                                }
                                fclose(f2);
                            }
                            fclose(f);
                        }
                    }
                }
            }
        }
        closedir(dir);
    }else{
        closedir(dir);
    }
    return fileCounter;
}

/**
  * @brief get the free space of SPIFFS storage in percent
  * 
  * @return -1=error, bookmark stored, >=0, 0...100 percent
  */
int32_t SAVES_getUsedSpaceLevel(){
    size_t total = 0, used = 0;
//    if ((esp_spiffs_info(SAVES_spiffsConf.partition_label, &total, &used) != ESP_OK)||(used>total)) {
//        ESP_LOGE(SAVES_LOG_TAG, "Failed to get SPIFFS partition information.");
        return -1;
//    }
    //ESP_LOGI(SAVES_LOG_TAG, "Used: %d, total %d",used,total);
    return used*100/total;
}
#endif
