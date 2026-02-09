#include "hormann_bisecur_scene.h"
#include <input/input.h>

static void
    hormann_bisecur_scene_menu_callback_ex(void* context, InputType input_type, uint32_t index) {
    HormannApp* app = context;
    if(input_type != InputTypeShort && input_type != InputTypeLong) return;

    if(index < app->door_count) {
        app->selected_door = index;
        if(input_type == InputTypeLong) {
            view_dispatcher_send_custom_event(
                app->view_dispatcher, HormannCustomEventDeleteDoor);
        } else {
            view_dispatcher_send_custom_event(
                app->view_dispatcher, HormannCustomEventMenuSelected);
        }
    } else if(input_type == InputTypeShort) {
        view_dispatcher_send_custom_event(app->view_dispatcher, HormannCustomEventAddDoor);
    }
}

void hormann_bisecur_scene_menu_on_enter(void* context) {
    HormannApp* app = context;
    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Hormann BiSecur");

    for(uint8_t i = 0; i < app->door_count; i++) {
        submenu_add_item_ex(
            app->submenu, app->doors[i].name, i, hormann_bisecur_scene_menu_callback_ex, app);
    }
    submenu_add_item_ex(
        app->submenu,
        "Add Door",
        app->door_count,
        hormann_bisecur_scene_menu_callback_ex,
        app);

    uint32_t state = scene_manager_get_scene_state(app->scene_manager, HormannSceneMenu);
    submenu_set_selected_item(app->submenu, state);

    view_dispatcher_switch_to_view(app->view_dispatcher, HormannViewMenu);
}

bool hormann_bisecur_scene_menu_on_event(void* context, SceneManagerEvent event) {
    HormannApp* app = context;
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == HormannCustomEventMenuSelected) {
            scene_manager_set_scene_state(
                app->scene_manager, HormannSceneMenu, app->selected_door);
            scene_manager_next_scene(app->scene_manager, HormannSceneControl);
            return true;
        } else if(event.event == HormannCustomEventAddDoor) {
            scene_manager_next_scene(app->scene_manager, HormannSceneAdd);
            return true;
        } else if(event.event == HormannCustomEventDeleteDoor) {
            scene_manager_next_scene(app->scene_manager, HormannSceneConfirmDelete);
            return true;
        }
    }
    return false;
}

void hormann_bisecur_scene_menu_on_exit(void* context) {
    HormannApp* app = context;
    submenu_reset(app->submenu);
}
