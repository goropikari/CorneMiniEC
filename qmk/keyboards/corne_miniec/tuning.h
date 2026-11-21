// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

#define EC_ROWS_PER_HAND (MATRIX_ROWS / 2)
#define EC_KEYS_PER_HAND (EC_ROWS_PER_HAND * MATRIX_COLS)

typedef struct {
  uint16_t press;
  uint16_t release;
} ec_offsets_t;

typedef struct {
  uint16_t sample;
  uint16_t baseline;
} ec_reading_t;

void ec_tuning_init(void);
ec_offsets_t ec_tuning_offsets(uint8_t index);
ec_reading_t ec_matrix_reading(uint8_t index);
