#include "hormann_bisecur_store.h"

#include <furi.h>
#include <storage/storage.h>
#include <flipper_format/flipper_format.h>

#define HORMANN_CONFIG_PATH     APP_DATA_PATH("config.txt")
#define HORMANN_CONFIG_FILETYPE "Hormann BiSecur Config"
#define HORMANN_CONFIG_VERSION  1

bool hormann_bisecur_store_load(HormannApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_file_alloc(storage);
    bool success = false;

    app->door_count = 0;

    do {
        if(!flipper_format_file_open_existing(ff, HORMANN_CONFIG_PATH)) break;

        FuriString* filetype = furi_string_alloc();
        uint32_t version = 0;
        if(!flipper_format_read_header(ff, filetype, &version)) {
            furi_string_free(filetype);
            break;
        }
        if(furi_string_cmp_str(filetype, HORMANN_CONFIG_FILETYPE) != 0 ||
           version != HORMANN_CONFIG_VERSION) {
            furi_string_free(filetype);
            break;
        }
        furi_string_free(filetype);

        FuriString* name = furi_string_alloc();
        while(app->door_count < HORMANN_MAX_DOORS) {
            if(!flipper_format_read_string(ff, "Door", name)) break;

            uint32_t serial = 0;
            uint32_t counter = 0;
            uint8_t aes_key[HORMANN_AES_KEY_SIZE];
            if(!flipper_format_read_uint32(ff, "Serial", &serial, 1)) break;
            if(!flipper_format_read_hex(ff, "AES_Key", aes_key, HORMANN_AES_KEY_SIZE)) break;
            if(!flipper_format_read_uint32(ff, "Counter", &counter, 1)) break;

            HormannDoor* door = &app->doors[app->door_count];
            strncpy(door->name, furi_string_get_cstr(name), HORMANN_NAME_MAX - 1);
            door->name[HORMANN_NAME_MAX - 1] = '\0';
            door->serial = serial;
            memcpy(door->aes_key, aes_key, HORMANN_AES_KEY_SIZE);
            door->counter = counter;
            app->door_count++;
        }
        furi_string_free(name);
        success = true;
    } while(false);

    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);
    return success;
}

bool hormann_bisecur_store_save(HormannApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_file_alloc(storage);
    bool success = false;

    do {
        if(!storage_simply_mkdir(storage, APP_DATA_PATH(""))) {
            // Directory may already exist, continue
        }

        if(!flipper_format_file_open_always(ff, HORMANN_CONFIG_PATH)) break;
        if(!flipper_format_write_header_cstr(
               ff, HORMANN_CONFIG_FILETYPE, HORMANN_CONFIG_VERSION))
            break;

        bool write_ok = true;
        for(uint8_t i = 0; i < app->door_count; i++) {
            HormannDoor* door = &app->doors[i];
            uint32_t serial = door->serial;
            uint32_t counter = door->counter;

            if(!flipper_format_write_string_cstr(ff, "Door", door->name)) {
                write_ok = false;
                break;
            }
            if(!flipper_format_write_uint32(ff, "Serial", &serial, 1)) {
                write_ok = false;
                break;
            }
            if(!flipper_format_write_hex(ff, "AES_Key", door->aes_key, HORMANN_AES_KEY_SIZE)) {
                write_ok = false;
                break;
            }
            if(!flipper_format_write_uint32(ff, "Counter", &counter, 1)) {
                write_ok = false;
                break;
            }
        }
        if(!write_ok) break;
        success = true;
    } while(false);

    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);
    return success;
}
