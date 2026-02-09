#pragma once

#include "../hormann_bisecur.h"
#include <gui/scene_manager.h>

void hormann_bisecur_scene_control_on_enter(void* context);
void hormann_bisecur_scene_add_on_enter(void* context);

bool hormann_bisecur_scene_control_on_event(void* context, SceneManagerEvent event);
bool hormann_bisecur_scene_add_on_event(void* context, SceneManagerEvent event);

void hormann_bisecur_scene_control_on_exit(void* context);
void hormann_bisecur_scene_add_on_exit(void* context);

extern const AppSceneOnEnterCallback hormann_bisecur_scene_on_enter_handlers[];
extern const AppSceneOnEventCallback hormann_bisecur_scene_on_event_handlers[];
extern const AppSceneOnExitCallback hormann_bisecur_scene_on_exit_handlers[];
extern const SceneManagerHandlers hormann_bisecur_scene_handlers;
