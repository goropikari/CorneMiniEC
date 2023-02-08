# Scanner logic is based on https://github.com/sekigon-gonnoc/ec-switch-scan-module
import digitalio
from analogio import AnalogIn
from keypad import Event as KeyEvent
from storage import getmount
from kmk.scanners import Scanner
from kmk.modules.split import SplitSide

class ECMatrixScanner(Scanner):
    def __init__(
        self,
        col_channels,    # Array[Integer]
        rows,            # Array[Pin]
        mux_sels,        # Array[Pin]
        adc_port,        # Pin
        discharge_port,  # Pin
        low_threshold,   # int
        high_threshold,  # int
        debug=True
    ):
        self.col_channels = col_channels
        self.len_cols = len(col_channels)
        self.len_rows = len(rows)
        self.adc_pin = AnalogIn(adc_port)
        self.discharge_pin = digitalio.DigitalInOut(discharge_port)
        self.low_threshold = low_threshold
        self.high_threshold = high_threshold
        self.key = 0  # scan する key の番号
        self.niter = 0
        self.debug = debug
        self.vals = [0 for i in range(self.len_cols * self.len_rows)]

        self.keynum_offset = 0
        side = SplitSide.RIGHT if str(getmount('/').label)[-1] == 'R' else SplitSide.LEFT
        if side == SplitSide.RIGHT:
            self.keynum_offset = self.len_cols * self.len_rows

        # A pin cannot be both a row and column, detect this by combining the
        # two tuples into a set and validating that the length did not drop
        #
        # repr() hackery is because CircuitPython Pin objects are not hashable
        unique_pins = {repr(c) for c in col_channels} | {repr(r) for r in rows}
        assert (
            len(unique_pins) == self.len_cols + self.len_rows
        ), 'Cannot use a pin as both a column and row'
        del unique_pins

        # __class__.__name__ is used instead of isinstance as the MCP230xx lib
        # does not use the digitalio.DigitalInOut, but rather a self defined one:
        # https://github.com/adafruit/Adafruit_CircuitPython_MCP230xx/blob/3f04abbd65ba5fa938fcb04b99e92ae48a8c9406/adafruit_mcp230xx/digital_inout.py#L33

        self.rows = [
            x
            if x.__class__.__name__ == 'DigitalInOut'
            else digitalio.DigitalInOut(x)
            for x in rows
        ]
        self.mux_sels = [
            x
            if x.__class__.__name__ == 'DigitalInOut'
            else digitalio.DigitalInOut(x)
            for x in mux_sels
        ]

        self._key_count = self.len_cols * self.len_rows
        self.states = bytearray(self.key_count * 2)

        self.ecsm_init()

    @property
    def key_count(self):
        return self._key_count

    def scan_for_changes(self):
        any_changed = False

        cidx = self.key % self.len_cols
        ridx = self.key // self.len_cols

        val = self.ecsm_readkey_raw(ridx, cidx)
        any_changed |= self.ecsm_update_key(val, ridx, cidx)

        self.vals[ridx * self.len_cols + cidx] = val

        self.key += 1
        self.key %= self.key_count
        self.niter += 1
        self.niter %= 50
        if self.debug:
            if self.key == 0 and self.niter == 0:
                print('(' + ",".join(map(lambda i: str(i), self.vals)) + ')')
            return None

        if any_changed:
            n = self.key_number(ridx, cidx)
            pressed = int(self.states[n])
            return KeyEvent(n, pressed)

        return None

    def select_mux(self, col):
        ch = self.col_channels[col]
        self.mux_sels[0].value = bool(ch & 1)
        self.mux_sels[1].value = bool(ch & 2)
        self.mux_sels[2].value = bool(ch & 4)

    def discharge_capacitor(self):
        self.discharge_pin.switch_to_output()
        self.discharge_pin.value = False

    def charge_capacitor(self, ridx):
        self.discharge_pin.switch_to_input()
        self.rows[ridx].value = True

    def clear_all_row_pins(self):
        for row in self.rows:
            row.switch_to_output()
            row.value = False

    def init_mux_sel(self):
        for sel in self.mux_sels:
            sel.switch_to_output()
            sel.value = False

    def ecsm_init(self):
        self.clear_all_row_pins()
        self.init_mux_sel()
        self.discharge_capacitor()

    def ecsm_readkey_raw(self, ridx, cidx):
        sw_value = 0
        self.clear_all_row_pins()
        self.discharge_capacitor()
        self.select_mux(cidx)
        self.charge_capacitor(ridx)

        sw_value = (self.adc_pin.value * 3.3) / 65536
        return sw_value

    def ecsm_update_key(self, sw_value, row, col):
        n = self.key_number(row, col)
        current_state = self.states[n]

        if current_state and sw_value < self.low_threshold:
            self.states[n] = 0
            return True

        if not current_state and sw_value > self.high_threshold:
            self.states[n] = 1
            return True

        return False

    def key_number(self, ridx, cidx):
        return ridx * self.len_cols + cidx + self.keynum_offset
