// Copyright 2020 sekigon-gonnoc
// Copyright 2022 goropikari
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#define MATRIX_ROWS 6
#define MATRIX_COLS 6

// With no VBUS sense pin, determine the master by USB enumeration.
#define SPLIT_USB_DETECT
#define EE_HANDS
#define SERIAL_USART_TX_PIN GP1

#define ADC_RESOLUTION 12
#define EC_ADC_PIN GP26
#define EC_DISCHARGE_PIN GP27
#define EC_DRIVE_PINS {GP6, GP7, GP0}
#define EC_MUX_PINS {GP3, GP4, GP2}

// Default release and press offsets in 12-bit ADC units.
#define EC_RELEASE_OFFSET 480
#define EC_PRESS_OFFSET 560
#define EC_DISCHARGE_US 5
#define EC_ADC_SETTLE_US 1

// Store a press/release pair of 16-bit offsets for each local key.
#define EECONFIG_KB_DATA_SIZE (MATRIX_ROWS / 2 * MATRIX_COLS * 4)
#define EECONFIG_KB_DATA_VERSION 0x43450001UL

// KMK's absolute thresholds are used by the scan before calibration
// (Bootmagic).
#define EC_STARTUP_RELEASE_THRESHOLD 745
#define EC_STARTUP_PRESS_THRESHOLD 869

#define TAP_CODE_DELAY 5
#define TAPPING_TERM 125
#define PERMISSIVE_HOLD

#ifdef VIAL_ENABLE
#define VIAL_KEYBOARD_UID {0x62, 0xD1, 0x38, 0x7C, 0x8F, 0xE5, 0x7C, 0xDD}
// Top-left and bottom-right letter keys of the left half.
#define VIAL_UNLOCK_COMBO_ROWS {0, 2}
#define VIAL_UNLOCK_COMBO_COLS {4, 0}
#endif
