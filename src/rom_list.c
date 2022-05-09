#include "rom_list.h"
#include "pd_api.h"

extern PlaydateAPI *pd;

char **file_list;
int max_files = 10;
int num_files = 0;
int selected_file = 0;
LCDFont* font;

void *file_list_callback(const char *filename, void *userdata)
{
    int len = strlen(filename);
    if (strcmp(filename + len - 3, ".gb") == 0 || strcmp(filename + len - 4, ".gbc") == 0) {
        if(num_files + 1 > max_files) {
            max_files += 10;
            file_list = pd->system->realloc(file_list, max_files * sizeof(char*));
        }
        file_list[num_files] = pd->system->realloc(NULL, (len + 1) * sizeof(char));
        strcpy(file_list[num_files], filename);
        num_files++;
    }
}

void rom_list_init()
{
    font = pd->graphics->loadFont("Asheville-Sans-14-Bold", NULL);
    pd->graphics->setFont(font);
    file_list = pd->system->realloc(NULL, max_files * sizeof(char*));
    pd->file->listfiles(".", file_list_callback, NULL);

    if(num_files == 0) {
        pd->file->open("place_roms_here.txt", kFileWrite);
    }

    pd->system->logToConsole("Found ROMs:");
    for(int i = 0; i < num_files; i++) {
        pd->system->logToConsole(file_list[i]);
    }
}

void redraw_menu_screen() {
    pd->graphics->clear(kColorWhite);

    if(num_files == 0) {
        pd->graphics->drawText("Place ROMs in \"Data/com.timhei.peanutGB\" folder", 47, kASCIIEncoding, 20, 110);
    }

    for(int i = 0; i < 8; i++) {
        if (i >= num_files) break;
        int selected_file_offset= selected_file - 7;
        if(selected_file_offset < 0) selected_file_offset = 0;

        if(i == selected_file) {
            pd->graphics->fillRect(0, i * 30, 400, 30, kColorBlack);
            pd->graphics->setDrawMode(kDrawModeFillWhite);
            pd->graphics->drawText(file_list[i], strlen(file_list[i]), kASCIIEncoding, 20, (i * 30) + 7);
            pd->graphics->setDrawMode(kDrawModeFillBlack);
        }
        else if(selected_file > 7 && i == 7){
            pd->graphics->fillRect(0, i * 30, 400, 30, kColorBlack);
            pd->graphics->setDrawMode(kDrawModeFillWhite);
            pd->graphics->drawText(file_list[i + selected_file_offset], strlen(file_list[i + selected_file_offset]), kASCIIEncoding, 20, (i * 30) + 7);
            pd->graphics->setDrawMode(kDrawModeFillBlack);
        }
        else if(selected_file > 7){
            pd->graphics->drawText(file_list[i + selected_file_offset], strlen(file_list[i + selected_file_offset]), kASCIIEncoding, 20, (i * 30) + 7);
        }
        else {
            pd->graphics->drawText(file_list[i], strlen(file_list[i]), kASCIIEncoding, 20, (i * 30) + 7);
        }
    }
}

char * rom_list_update()
{
    PDButtons pushed;
    pd->system->getButtonState(NULL, &pushed, NULL);

    if(num_files > 0 && pushed) {
        if ( pushed & kButtonA ) {
            char *selected_file_copy = pd->system->realloc(NULL, (strlen(file_list[selected_file]) + 1) * sizeof(char));
            strcpy(selected_file_copy, file_list[selected_file]);
            return selected_file_copy;
        }
        if ( pushed & kButtonUp ) {
            selected_file--;
            if(selected_file < 0) {
                selected_file = num_files - 1;
            }
            redraw_menu_screen();
        }
        if ( pushed & kButtonDown ) {
            selected_file++;
            if(selected_file > num_files - 1) {
                selected_file = 0;
            }
            redraw_menu_screen();
        }
    }

    return NULL;
}

void rom_list_cleanup() {
    for(int i = 0; i < num_files; i++) {
        if(file_list[i] != NULL) pd->system->realloc(file_list[i], 0);
    }
    if(file_list != NULL) pd->system->realloc(file_list, 0);
}