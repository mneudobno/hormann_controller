#include "hormann_bisecur_scene.h"
#include "../hormann_bisecur_protocol.h"
#include "../hormann_bisecur_store.h"
#include <gui/canvas.h>

static void hormann_bisecur_scene_control_send(HormannApp* app, HormannCmd cmd) {
    HormannDoor* door = &app->doors[app->selected_door];
    hormann_bisecur_send(door->serial, door->aes_key, door->counter, cmd);
    door->counter++;
    hormann_bisecur_store_save(app);
}

static void hormann_bisecur_scene_control_draw(Canvas* canvas, void* model) {
    HormannApp* app = *(HormannApp**)model;
    HormannDoor* door = &app->doors[app->selected_door];

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 2, AlignCenter, AlignTop, door->name);

    char buf[48];
    canvas_set_font(canvas, FontSecondary);
    snprintf(buf, sizeof(buf), "Serial: %08lX  Cnt: %lu", door->serial, door->counter);
    canvas_draw_str_aligned(canvas, 64, 16, AlignCenter, AlignTop, buf);

    canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignTop, "[UP] Open");
    canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignTop, "[OK] Stop  [DOWN] Close");
    canvas_draw_str_aligned(canvas, 64, 50, AlignCenter, AlignTop, "Hold OK = Light");
}

static bool hormann_bisecur_scene_control_input(InputEvent* event, void* context) {
    HormannApp* app = context;

    if(event->key == InputKeyBack) {
        return false;
    }

    if(event->type == InputTypeShort) {
        switch(event->key) {
        case InputKeyUp:
            hormann_bisecur_scene_control_send(app, HormannCmdOpen);
            view_commit_model(app->control_view, true);
            return true;
        case InputKeyDown:
            hormann_bisecur_scene_control_send(app, HormannCmdClose);
            view_commit_model(app->control_view, true);
            return true;
        case InputKeyOk:
            hormann_bisecur_scene_control_send(app, HormannCmdStop);
            view_commit_model(app->control_view, true);
            return true;
        default:
            break;
        }
    } else if(event->type == InputTypeLong && event->key == InputKeyOk) {
        hormann_bisecur_scene_control_send(app, HormannCmdLight);
        view_commit_model(app->control_view, true);
        return true;
    }

    return false;
}

void hormann_bisecur_scene_control_on_enter(void* context) {
    HormannApp* app = context;
    view_set_draw_callback(app->control_view, hormann_bisecur_scene_control_draw);
    view_set_input_callback(app->control_view, hormann_bisecur_scene_control_input);
    view_set_context(app->control_view, app);
    view_commit_model(app->control_view, true);
    view_dispatcher_switch_to_view(app->view_dispatcher, HormannViewControl);
}

bool hormann_bisecur_scene_control_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void hormann_bisecur_scene_control_on_exit(void* context) {
    UNUSED(context);
}
