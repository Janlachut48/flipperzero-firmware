#include "../subghz_i.h"

typedef enum {
    SubGhzSceneReceiverInfoCustomEventTxStart,
    SubGhzSceneReceiverInfoCustomEventTxStop,
    SubGhzSceneReceiverInfoCustomEventSave,
} SubGhzSceneReceiverInfoCustomEvent;

void subghz_scene_receiver_info_callback(GuiButtonType result, InputType type, void* context) {
    furi_assert(context);
    SubGhz* subghz = context;

    if((result == GuiButtonTypeCenter) && (type == InputTypePress)) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzSceneReceiverInfoCustomEventTxStart);
    } else if((result == GuiButtonTypeCenter) && (type == InputTypeRelease)) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzSceneReceiverInfoCustomEventTxStop);
    } else if((result == GuiButtonTypeRight) && (type == InputTypeShort)) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzSceneReceiverInfoCustomEventSave);
    }
}

static bool subghz_scene_receiver_info_update_parser(void* context) {
    SubGhz* subghz = context;
    subghz->txrx->protocol_result = subghz_parser_get_by_name(
        subghz->txrx->parser,
        subghz_history_get_name(subghz->txrx->history, subghz->txrx->idx_menu_chosen));

    if(subghz->txrx->protocol_result->to_load_protocol != NULL) {
        subghz->txrx->protocol_result->to_load_protocol(
            subghz->txrx->protocol_result,
            subghz_history_get_raw_data(subghz->txrx->history, subghz->txrx->idx_menu_chosen));
        subghz->txrx->frequency =
            subghz_history_get_frequency(subghz->txrx->history, subghz->txrx->idx_menu_chosen);
        subghz->txrx->preset =
            subghz_history_get_preset(subghz->txrx->history, subghz->txrx->idx_menu_chosen);
        return true;
    }
    return false;
}

void subghz_scene_receiver_info_on_enter(void* context) {
    SubGhz* subghz = context;

    if(subghz_scene_receiver_info_update_parser(subghz)) {
        char buffer_str[16];
        snprintf(
            buffer_str,
            sizeof(buffer_str),
            "%03ld.%02ld",
            subghz->txrx->frequency / 1000000 % 1000,
            subghz->txrx->frequency / 10000 % 100);
        widget_add_string_element(
            subghz->widget, 78, 0, AlignLeft, AlignTop, FontSecondary, buffer_str);
        if(subghz->txrx->preset == FuriHalSubGhzPresetOok650Async ||
           subghz->txrx->preset == FuriHalSubGhzPresetOok270Async) {
            snprintf(buffer_str, sizeof(buffer_str), "AM");
        } else if(subghz->txrx->preset == FuriHalSubGhzPreset2FSKAsync) {
            snprintf(buffer_str, sizeof(buffer_str), "FM");
        } else {
            furi_crash(NULL);
        }
        widget_add_string_element(
            subghz->widget, 113, 0, AlignLeft, AlignTop, FontSecondary, buffer_str);
        string_t text;
        string_init(text);
        subghz->txrx->protocol_result->to_string(subghz->txrx->protocol_result, text);
        widget_add_string_multiline_element(
            subghz->widget, 0, 0, AlignLeft, AlignTop, FontSecondary, string_get_cstr(text));
        string_clear(text);

        if(subghz->txrx->protocol_result && subghz->txrx->protocol_result->to_save_string &&
           strcmp(subghz->txrx->protocol_result->name, "KeeLoq")) {
            widget_add_button_element(
                subghz->widget,
                GuiButtonTypeRight,
                "Save",
                subghz_scene_receiver_info_callback,
                subghz);
            widget_add_button_element(
                subghz->widget,
                GuiButtonTypeCenter,
                "Send",
                subghz_scene_receiver_info_callback,
                subghz);
        }

    } else {
        widget_add_icon_element(subghz->widget, 32, 12, &I_DolphinFirstStart7_61x51);
        widget_add_string_element(
            subghz->widget, 13, 8, AlignLeft, AlignBottom, FontSecondary, "Error history parse.");
    }

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewWidget);
}

bool subghz_scene_receiver_info_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubGhzSceneReceiverInfoCustomEventTxStart) {
            //CC1101 Stop RX -> Start TX
            subghz->state_notifications = NOTIFICATION_TX_STATE;
            if(subghz->txrx->hopper_state != SubGhzHopperStateOFF) {
                subghz->txrx->hopper_state = SubGhzHopperStatePause;
            }
            if(subghz->txrx->txrx_state == SubGhzTxRxStateRx) {
                subghz_rx_end(subghz);
            }
            if(!subghz_scene_receiver_info_update_parser(subghz)) {
                return false;
            }
            if(subghz->txrx->txrx_state == SubGhzTxRxStateIdle) {
                subghz_tx_start(subghz);
            }
            return true;
        } else if(event.event == SubGhzSceneReceiverInfoCustomEventTxStop) {
            //CC1101 Stop Tx -> Start RX
            subghz->state_notifications = NOTIFICATION_IDLE_STATE;
            if(subghz->txrx->txrx_state == SubGhzTxRxStateTx) {
                subghz_tx_stop(subghz);
            }
            if(subghz->txrx->txrx_state == SubGhzTxRxStateIdle) {
                subghz_begin(subghz, subghz->txrx->preset);
                subghz_rx(subghz, subghz->txrx->frequency);
            }
            if(subghz->txrx->hopper_state == SubGhzHopperStatePause) {
                subghz->txrx->hopper_state = SubGhzHopperStateRunnig;
            }
            subghz->state_notifications = NOTIFICATION_RX_STATE;
            return true;
        } else if(event.event == SubGhzSceneReceiverInfoCustomEventSave) {
            //CC1101 Stop RX -> Save
            subghz->state_notifications = NOTIFICATION_IDLE_STATE;
            if(subghz->txrx->hopper_state != SubGhzHopperStateOFF) {
                subghz->txrx->hopper_state = SubGhzHopperStateOFF;
            }
            if(subghz->txrx->txrx_state == SubGhzTxRxStateRx) {
                subghz_rx_end(subghz);
                subghz_sleep(subghz);
            }
            if(!subghz_scene_receiver_info_update_parser(subghz)) {
                return false;
            }
            if(subghz->txrx->protocol_result && subghz->txrx->protocol_result->to_save_string &&
               strcmp(subghz->txrx->protocol_result->name, "KeeLoq")) {
                scene_manager_next_scene(subghz->scene_manager, SubGhzSceneSaveName);
            }
            return true;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        if(subghz->txrx->hopper_state != SubGhzHopperStateOFF) {
            subghz_hopper_update(subghz);
        }
        switch(subghz->state_notifications) {
        case NOTIFICATION_TX_STATE:
            notification_message(subghz->notifications, &sequence_blink_red_10);
            break;
        case NOTIFICATION_RX_STATE:
            notification_message(subghz->notifications, &sequence_blink_blue_10);
            break;
        default:
            break;
        }
    }
    return false;
}

void subghz_scene_receiver_info_on_exit(void* context) {
    SubGhz* subghz = context;
    widget_clear(subghz->widget);
}