#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/view.h>
#include <gui/modules/text_input.h>

#define HORMANN_NAME_MAX     32
#define HORMANN_AES_KEY_SIZE 16

typedef struct {
    char name[HORMANN_NAME_MAX];
    uint32_t serial;
    uint8_t aes_key[HORMANN_AES_KEY_SIZE];
    uint32_t counter;
} HormannDoor;

typedef enum {
    HormannViewControl,
    HormannViewTextInput,
} HormannView;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    View* control_view;
    TextInput* text_input;
    HormannDoor door;
    bool door_configured;
    char text_buf[HORMANN_NAME_MAX];
} HormannApp;

typedef enum {
    HormannSceneControl,
    HormannSceneAdd,
    HormannSceneCount,
} HormannScene;

typedef enum {
    HormannCustomEventTextInputDone,
} HormannCustomEvent;
