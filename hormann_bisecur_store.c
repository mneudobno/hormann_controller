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

    app->door_configured = false;

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
        if(!flipper_format_read_string(ff, "Door", name)) {
            furi_string_free(name);
            break;
        }

        uint32_t serial = 0;
        uint32_t counter = 0;
        uint8_t aes_key[HORMANN_AES_KEY_SIZE];
        if(!flipper_format_read_uint32(ff, "Serial", &serial, 1)) {
            furi_string_free(name);
            break;
        }
        if(!flipper_format_read_hex(ff, "AES_Key", aes_key, HORMANN_AES_KEY_SIZE)) {
            furi_string_free(name);
            break;
        }
        if(!flipper_format_read_uint32(ff, "Counter", &counter, 1)) {
            furi_string_free(name);
            break;
        }

        strncpy(app->door.name, furi_string_get_cstr(name), HORMANN_NAME_MAX - 1);
        app->door.name[HORMANN_NAME_MAX - 1] = '\0';
        app->door.serial = serial;
        memcpy(app->door.aes_key, aes_key, HORMANN_AES_KEY_SIZE);
        app->door.counter = counter;
        app->door_configured = true;
        furi_string_free(name);
    } while(false);

    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);
    return app->door_configured;
}

bool hormann_bisecur_store_save(HormannApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_file_alloc(storage);
    bool success = false;

    do {
        storage_simply_mkdir(storage, APP_DATA_PATH(""));

        if(!flipper_format_file_open_always(ff, HORMANN_CONFIG_PATH)) break;
        if(!flipper_format_write_header_cstr(
               ff, HORMANN_CONFIG_FILETYPE, HORMANN_CONFIG_VERSION))
            break;

        uint32_t serial = app->door.serial;
        uint32_t counter = app->door.counter;

        if(!flipper_format_write_string_cstr(ff, "Door", app->door.name)) break;
        if(!flipper_format_write_uint32(ff, "Serial", &serial, 1)) break;
        if(!flipper_format_write_hex(ff, "AES_Key", app->door.aes_key, HORMANN_AES_KEY_SIZE))
            break;
        if(!flipper_format_write_uint32(ff, "Counter", &counter, 1)) break;
        success = true;
    } while(false);

    flipper_format_free(ff);
    furi_record_close(RECORD_STORAGE);
    return success;
}
