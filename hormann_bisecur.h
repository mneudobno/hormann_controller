#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>
#include <gui/view.h>
#include <gui/modules/text_input.h>

#define HORMANN_MAX_DOORS    8
#define HORMANN_NAME_MAX     32
#define HORMANN_AES_KEY_SIZE 16

typedef struct {
    char name[HORMANN_NAME_MAX];
    uint32_t serial;
    uint8_t aes_key[HORMANN_AES_KEY_SIZE];
    uint32_t counter;
} HormannDoor;

typedef enum {
    HormannViewMenu,
    HormannViewControl,
    HormannViewTextInput,
    HormannViewConfirmDelete,
} HormannView;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    Submenu* submenu;
    View* control_view;
    TextInput* text_input;
    DialogEx* dialog_ex;
    HormannDoor doors[HORMANN_MAX_DOORS];
    uint8_t door_count;
    uint8_t selected_door;
    char text_buf[HORMANN_NAME_MAX];
} HormannApp;

typedef enum {
    HormannSceneMenu,
    HormannSceneControl,
    HormannSceneAdd,
    HormannSceneConfirmDelete,
    HormannSceneCount,
} HormannScene;

typedef enum {
    HormannCustomEventMenuSelected,
    HormannCustomEventAddDoor,
    HormannCustomEventTextInputDone,
    HormannCustomEventDeleteDoor,
    HormannCustomEventDeleteConfirm,
    HormannCustomEventDeleteCancel,
} HormannCustomEvent;
