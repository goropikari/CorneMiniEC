// Copyright 2023 goropikari
// SPDX-License-Identifier: GPL-2.0-or-later

#include "tuning.h"

#include <stdbool.h>
#include <string.h>

#include "eeconfig.h"
#include "keyboard.h"
#include "via.h"

#define REPORT_SIZE 32
#define RESPONSE_OFFSET 6
#define ACKNOWLEDGMENT 0xA5
#define PAGE_KEYS 6
#define MAX_OFFSET ((1U << ADC_RESOLUTION) - 2U)

enum tuning_command {
    TUNING_INFO = 1,
    TUNING_READ,
    TUNING_APPLY,
    TUNING_SAVE,
    TUNING_READ_SAVED,
    TUNING_RESTORE,
};

static ec_offsets_t offsets[EC_KEYS_PER_HAND];
static ec_offsets_t staged_offsets[EC_KEYS_PER_HAND];
static uint8_t next_restore_key;
static uint8_t next_restore_id;

_Static_assert(sizeof(offsets) == EECONFIG_KB_DATA_SIZE, "Wrong tuning storage size");
_Static_assert(EC_KEYS_PER_HAND % PAGE_KEYS == 0, "Incomplete tuning page");
_Static_assert(RESPONSE_OFFSET + PAGE_KEYS * 4 <= REPORT_SIZE - 1, "Tuning page does not fit");
_Static_assert(0 < EC_RELEASE_OFFSET && EC_RELEASE_OFFSET < EC_PRESS_OFFSET && EC_PRESS_OFFSET <= MAX_OFFSET, "Invalid default offsets");

static bool valid_offsets(ec_offsets_t pair) {
    return pair.release > 0 && pair.release < pair.press && pair.press <= MAX_OFFSET;
}

static void load_offsets(ec_offsets_t *pairs, uint8_t first, uint8_t count) {
    const uint32_t size = count * sizeof(*pairs);
    if (eeconfig_read_kb_datablock(pairs, first * sizeof(*pairs), size) != size) {
        memset(pairs, 0, size);
    }

    for (uint8_t key = 0; key < count; ++key) {
        if (!valid_offsets(pairs[key])) {
            pairs[key] = (ec_offsets_t){EC_PRESS_OFFSET, EC_RELEASE_OFFSET};
        }
    }
}

static bool save_offsets(const ec_offsets_t *pairs) {
    ec_offsets_t saved[EC_KEYS_PER_HAND];
    return eeconfig_update_kb_datablock(pairs, 0, sizeof(saved)) == sizeof(saved) &&
           eeconfig_read_kb_datablock(saved, 0, sizeof(saved)) == sizeof(saved) &&
           memcmp(saved, pairs, sizeof(saved)) == 0;
}

void ec_tuning_init(void) {
    load_offsets(offsets, 0, EC_KEYS_PER_HAND);
}

ec_offsets_t ec_tuning_offsets(uint8_t index) {
    return offsets[index];
}

static uint16_t read_u16(const uint8_t *bytes) {
    return bytes[0] | ((uint16_t)bytes[1] << 8);
}

static void write_u16(uint8_t *bytes, uint16_t value) {
    bytes[0] = value & 0xFF;
    bytes[1] = value >> 8;
}

static ec_offsets_t decode_offsets(const uint8_t *bytes) {
    return (ec_offsets_t){read_u16(bytes), read_u16(bytes + 2)};
}

static void encode_offsets(uint8_t *bytes, ec_offsets_t pair) {
    write_u16(bytes, pair.press);
    write_u16(bytes + 2, pair.release);
}

static bool valid_page(uint8_t first) {
    return first < EC_KEYS_PER_HAND && first % PAGE_KEYS == 0;
}

static bool restore_page(uint8_t first, uint8_t request_id, const uint8_t *bytes) {
    const bool expected = first == 0 || (next_restore_key == first && next_restore_id == request_id);
    next_restore_key = 0;
    if (!valid_page(first) || !expected) {
        return false;
    }

    for (uint8_t i = 0; i < PAGE_KEYS; ++i) {
        ec_offsets_t pair = decode_offsets(bytes + i * 4);
        if (!valid_offsets(pair)) {
            return false;
        }
        staged_offsets[first + i] = pair;
    }

    if (first + PAGE_KEYS < EC_KEYS_PER_HAND) {
        next_restore_key = first + PAGE_KEYS;
        next_restore_id = request_id + 1;
        return true;
    }

    if (!save_offsets(staged_offsets)) {
        return false;
    }

    memcpy(offsets, staged_offsets, sizeof(offsets));
    return true;
}

void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    if (length != REPORT_SIZE || data[0] != 'C' || data[1] != 'E' || data[2] != 1) {
        if (length != 0) {
            data[0] = id_unhandled;
        }
        return;
    }

    // Header: 'C', 'E', version, command, request ID. Replies use status at byte 5.
    const uint8_t command = data[3];
    const uint8_t index = data[5];
    uint8_t *response = data + RESPONSE_OFFSET;
    const ec_offsets_t pair = command == TUNING_APPLY ? decode_offsets(response) : (ec_offsets_t){0};
    const bool restored = command == TUNING_RESTORE && restore_page(index, data[4], response);
    if (command != TUNING_RESTORE) {
        next_restore_key = 0;
    }

    memset(data + 5, 0, REPORT_SIZE - 5);
    switch (command) {
        case TUNING_INFO:
            response[0] = is_keyboard_left() ? 0 : 1;
            response[1] = EC_ROWS_PER_HAND;
            response[2] = MATRIX_COLS;
            response[3] = PAGE_KEYS;
            encode_offsets(response + 4, (ec_offsets_t){EC_PRESS_OFFSET, EC_RELEASE_OFFSET});
            write_u16(response + 8, MAX_OFFSET);
            break;
        case TUNING_READ:
        case TUNING_APPLY:
            if (index >= EC_KEYS_PER_HAND || (command == TUNING_APPLY && !valid_offsets(pair))) {
                data[5] = 1;
                break;
            }
            if (command == TUNING_APPLY) {
                offsets[index] = pair;
            }
            ec_reading_t reading = ec_matrix_reading(index);
            write_u16(response, reading.sample);
            write_u16(response + 2, reading.baseline);
            encode_offsets(response + 4, offsets[index]);
            break;
        case TUNING_SAVE:
            data[5] = !save_offsets(offsets);
            break;
        case TUNING_READ_SAVED: {
            if (!valid_page(index)) {
                data[5] = 1;
                break;
            }
            ec_offsets_t saved[PAGE_KEYS];
            load_offsets(saved, index, PAGE_KEYS);
            for (uint8_t i = 0; i < PAGE_KEYS; ++i) {
                encode_offsets(response + i * 4, saved[i]);
            }
            break;
        }
        case TUNING_RESTORE:
            data[5] = !restored;
            break;
        default:
            data[5] = 1;
            break;
    }

    // Have the browser ignore echoes while Vial is unlocking. VIA sends the reply.
    data[REPORT_SIZE - 1] = ACKNOWLEDGMENT;
}
