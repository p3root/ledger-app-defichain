/*****************************************************************************
 *   Ledger App Bitcoin.
 *   (c) 2021 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdint.h>  // uint*_t

#include "boilerplate/io.h"
#include "boilerplate/dispatcher.h"
#include "boilerplate/sw.h"
#include "../constants.h"
#include "../types.h"
#include "../crypto.h"
#include "../ui/display.h"
#include "../ui/menu.h"


static void ui_action_validate_pubkey(bool choice);

int handler_get_pubkey(
    uint8_t p1,
    uint8_t p2,
    uint8_t lc,
    dispatcher_context_t *dispatcher_context
) {
    get_pubkey_state_t *state = (get_pubkey_state_t *)&G_command_state;

    if (p1 > 1 || p2 != 0) {
        return io_send_sw(SW_WRONG_P1P2);
    }
    if (lc < 1) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    // Device must be unlocked
    if (os_global_pin_is_validated() != BOLOS_UX_OK) {
        return io_send_sw(SW_SECURITY_STATUS_NOT_SATISFIED);
    }

    uint8_t bip32_path_len;
    buffer_read_u8(&dispatcher_context->read_buffer, &bip32_path_len);

    if (bip32_path_len > MAX_BIP32_PATH_STEPS) {
        return io_send_sw(SW_INCORRECT_DATA);
    }

    uint32_t bip32_path[MAX_BIP32_PATH_STEPS];
    if (!buffer_read_bip32_path(&dispatcher_context->read_buffer, bip32_path, bip32_path_len)) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    // TODO: check if path is ok to export, adapt UI to ask for confirmation if suspicious.


    get_serialized_extended_pubkey(bip32_path, bip32_path_len, state->serialized_pubkey_str);

    char path_str[MAX_SERIALIZED_BIP32_PATH_LENGTH + 1] = "Master key";
    if (bip32_path_len > 0) {
        bip32_path_format(bip32_path, bip32_path_len, path_str, sizeof(path_str));
    }

    if (p1 == 1) {
        return ui_display_pubkey(path_str, state->serialized_pubkey_str, ui_action_validate_pubkey);
    } else {
        return io_send_response(state->serialized_pubkey_str, strlen(state->serialized_pubkey_str), SW_OK);
    }
}

static void ui_action_validate_pubkey(bool choice) {
    get_pubkey_state_t *state = (get_pubkey_state_t *)&G_command_state;
    if (choice) {
        io_send_response(state->serialized_pubkey_str, strlen(state->serialized_pubkey_str), SW_OK);
    } else {
        io_send_sw(SW_DENY);
    }

    ui_menu_main();
}