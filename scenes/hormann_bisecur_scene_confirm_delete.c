#include "hormann_bisecur_scene.h"
#include "../hormann_bisecur_store.h"

static void hormann_bisecur_scene_confirm_delete_cb(DialogExResult result, void* context) {
    HormannApp* app = context;
    if(result == DialogExResultRight) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, HormannCustomEventDeleteConfirm);
    } else {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, HormannCustomEventDeleteCancel);
    }
}

void hormann_bisecur_scene_confirm_delete_on_enter(void* context) {
    HormannApp* app = context;
    HormannDoor* door = &app->doors[app->selected_door];

    dialog_ex_set_header(app->dialog_ex, "Delete Door?", 64, 0, AlignCenter, AlignTop);
    dialog_ex_set_text(app->dialog_ex, door->name, 64, 26, AlignCenter, AlignCenter);
    dialog_ex_set_left_button_text(app->dialog_ex, "Cancel");
    dialog_ex_set_right_button_text(app->dialog_ex, "Delete");
    dialog_ex_set_result_callback(app->dialog_ex, hormann_bisecur_scene_confirm_delete_cb);
    dialog_ex_set_context(app->dialog_ex, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, HormannViewConfirmDelete);
}

bool hormann_bisecur_scene_confirm_delete_on_event(void* context, SceneManagerEvent event) {
    HormannApp* app = context;
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == HormannCustomEventDeleteConfirm) {
            for(uint8_t i = app->selected_door; i < app->door_count - 1; i++) {
                app->doors[i] = app->doors[i + 1];
            }
            app->door_count--;
            hormann_bisecur_store_save(app);
            scene_manager_search_and_switch_to_previous_scene(
                app->scene_manager, HormannSceneMenu);
            return true;
        } else if(event.event == HormannCustomEventDeleteCancel) {
            scene_manager_previous_scene(app->scene_manager);
            return true;
        }
    }
    return false;
}

void hormann_bisecur_scene_confirm_delete_on_exit(void* context) {
    HormannApp* app = context;
    dialog_ex_reset(app->dialog_ex);
}
