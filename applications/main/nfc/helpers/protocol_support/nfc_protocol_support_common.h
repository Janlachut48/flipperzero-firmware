#pragma once

#include <nfc/nfc_device.h>
#include <nfc/protocols/nfc_generic_event.h>

#include "nfc_protocol_support_render_common.h"

#include "../nfc_custom_event.h"
#include "../../scenes/nfc_scene.h"
#include "../../nfc_app.h"

typedef enum {
    NfcProtocolFeatureNone = 0,
    NfcProtocolFeatureMoreData = 1UL << 0,
    NfcProtocolFeatureEmulateUid = 1UL << 1,
    NfcProtocolFeatureEmulateFull = 1UL << 2,
    NfcProtocolFeatureEditUid = 1UL << 3,
} NfcProtocolFeature;