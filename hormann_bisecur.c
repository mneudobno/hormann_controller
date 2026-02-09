#include "hormann_bisecur.h"
#include "hormann_bisecur_store.h"
#include "scenes/hormann_bisecur_scene.h"

const AppSceneOnEnterCallback hormann_bisecur_scene_on_enter_handlers[] = {
    [HormannSceneControl] = hormann_bisecur_scene_control_on_enter,
    [HormannSceneAdd] = hormann_bisecur_scene_add_on_enter,
};

const AppSceneOnEventCallback hormann_bisecur_scene_on_event_handlers[] = {
    [HormannSceneControl] = hormann_bisecur_scene_control_on_event,
    [HormannSceneAdd] = hormann_bisecur_scene_add_on_event,
};

const AppSceneOnExitCallback hormann_bisecur_scene_on_exit_handlers[] = {
    [HormannSceneControl] = hormann_bisecur_scene_control_on_exit,
    [HormannSceneAdd] = hormann_bisecur_scene_add_on_exit,
};

const SceneManagerHandlers hormann_bisecur_scene_handlers = {
    .on_enter_handlers = hormann_bisecur_scene_on_enter_handlers,
    .on_event_handlers = hormann_bisecur_scene_on_event_handlers,
    .on_exit_handlers = hormann_bisecur_scene_on_exit_handlers,
    .scene_num = HormannSceneCount,
};

static bool hormann_bisecur_custom_event_callback(void* context, uint32_t event) {
    HormannApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool hormann_bisecur_back_event_callback(void* context) {
    HormannApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static HormannApp* hormann_bisecur_app_alloc(void) {
    HormannApp* app = malloc(sizeof(HormannApp));
    memset(app, 0, sizeof(HormannApp));

    app->gui = furi_record_open(RECORD_GUI);

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, hormann_bisecur_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, hormann_bisecur_back_event_callback);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->scene_manager = scene_manager_alloc(&hormann_bisecur_scene_handlers, app);

    app->control_view = view_alloc();
    view_allocate_model(app->control_view, ViewModelTypeLockFree, sizeof(HormannApp*));
    HormannApp** model = view_get_model(app->control_view);
    *model = app;
    view_commit_model(app->control_view, false);
    view_dispatcher_add_view(
        app->view_dispatcher, HormannViewControl, app->control_view);

    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, HormannViewTextInput, text_input_get_view(app->text_input));

    hormann_bisecur_store_load(app);

    return app;
}

static void hormann_bisecur_app_free(HormannApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, HormannViewControl);
    view_dispatcher_remove_view(app->view_dispatcher, HormannViewTextInput);

    view_free(app->control_view);
    text_input_free(app->text_input);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_GUI);

    free(app);
}

int32_t hormann_bisecur_app(void* p) {
    UNUSED(p);

    HormannApp* app = hormann_bisecur_app_alloc();

    if(app->door_configured) {
        scene_manager_next_scene(app->scene_manager, HormannSceneControl);
    } else {
        scene_manager_next_scene(app->scene_manager, HormannSceneAdd);
    }
    view_dispatcher_run(app->view_dispatcher);

    hormann_bisecur_app_free(app);
    return 0;
}
