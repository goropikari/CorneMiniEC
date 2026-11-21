/* Copyright 2020 sekigon-gonnoc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "analog.h"
#include "gpio.h"
#include "matrix.h"
#include "util.h"
#include "wait.h"

static const pin_t drive_pins[] = EC_DRIVE_PINS;
// SEL0, SEL1, SEL2, from least to most significant selector bit.
static const pin_t mux_pins[] = EC_MUX_PINS;
// Module inputs S5, S6, S4, S1, S2, S3 map to matrix columns 0 through 5.
static const uint8_t mux_channels[] = {4, 5, 3, 0, 1, 2};

_Static_assert(ARRAY_SIZE(drive_pins) == MATRIX_ROWS / 2, "Invalid local row count");
_Static_assert(ARRAY_SIZE(mux_channels) == MATRIX_COLS, "Invalid column count");
_Static_assert(EC_RELEASE_THRESHOLD < EC_PRESS_THRESHOLD, "Invalid EC thresholds");

void matrix_init_custom(void) {
    gpio_write_pin_low(EC_DISCHARGE_PIN);
    gpio_set_pin_output(EC_DISCHARGE_PIN);

    for (uint8_t row = 0; row < ARRAY_SIZE(drive_pins); ++row) {
        gpio_write_pin_low(drive_pins[row]);
        gpio_set_pin_output(drive_pins[row]);
    }

    for (uint8_t bit = 0; bit < ARRAY_SIZE(mux_pins); ++bit) {
        gpio_write_pin_low(mux_pins[bit]);
        gpio_set_pin_output(mux_pins[bit]);
    }

    // Start the ADC before the first timed charge and read.
    (void)analogReadPin(EC_ADC_PIN);
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool changed = false;

    for (uint8_t col = 0; col < ARRAY_SIZE(mux_channels); ++col) {
        for (uint8_t bit = 0; bit < ARRAY_SIZE(mux_pins); ++bit) {
            gpio_write_pin(mux_pins[bit], (mux_channels[col] >> bit) & 1);
        }

        for (uint8_t row = 0; row < ARRAY_SIZE(drive_pins); ++row) {
            // The discharge pin is driven low between samples and floats during measurement.
            wait_us(EC_DISCHARGE_US);
            gpio_set_pin_input(EC_DISCHARGE_PIN);
            gpio_write_pin_high(drive_pins[row]);
            int16_t sample = analogReadPin(EC_ADC_PIN);
            gpio_write_pin_low(drive_pins[row]);
            gpio_set_pin_output(EC_DISCHARGE_PIN);

            matrix_row_t key_mask = MATRIX_ROW_SHIFTER << col;
            bool was_pressed = (current_matrix[row] & key_mask) != 0;
            if (was_pressed && sample < EC_RELEASE_THRESHOLD) {
                current_matrix[row] &= ~key_mask;
                changed = true;
            } else if (!was_pressed && sample >= EC_PRESS_THRESHOLD) {
                current_matrix[row] |= key_mask;
                changed = true;
            }
        }
    }

    return changed;
}
