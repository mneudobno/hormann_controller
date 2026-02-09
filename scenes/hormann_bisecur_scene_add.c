#include "hormann_bisecur_scene.h"
#include "../hormann_bisecur_store.h"
#include <furi_hal_random.h>

static void hormann_bisecur_scene_add_text_input_cb(void* context) {
    HormannApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, HormannCustomEventTextInputDone);
}

void hormann_bisecur_scene_add_on_enter(void* context) {
    HormannApp* app = context;
    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Door Name:");
    text_input_set_minimum_length(app->text_input, 1);
    app->text_buf[0] = '\0';
    text_input_set_result_callback(
        app->text_input,
        hormann_bisecur_scene_add_text_input_cb,
        app,
        app->text_buf,
        HORMANN_NAME_MAX,
        true);
    view_dispatcher_switch_to_view(app->view_dispatcher, HormannViewTextInput);
}

bool hormann_bisecur_scene_add_on_event(void* context, SceneManagerEvent event) {
    HormannApp* app = context;
    if(event.type == SceneManagerEventTypeCustom &&
       event.event == HormannCustomEventTextInputDone) {
        if(strlen(app->text_buf) > 0) {
            strncpy(app->door.name, app->text_buf, HORMANN_NAME_MAX - 1);
            app->door.name[HORMANN_NAME_MAX - 1] = '\0';
            app->door.serial = furi_hal_random_get();
            furi_hal_random_fill_buf(app->door.aes_key, HORMANN_AES_KEY_SIZE);
            app->door.counter = 1;
            app->door_configured = true;
            hormann_bisecur_store_save(app);
            scene_manager_next_scene(app->scene_manager, HormannSceneControl);
        }
        return true;
    }
    return false;
}

void hormann_bisecur_scene_add_on_exit(void* context) {
    HormannApp* app = context;
    text_input_reset(app->text_input);
}
