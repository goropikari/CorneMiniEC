import board
from kmk.kmk_keyboard import KMKKeyboard
from kmk.keys import KC
from kmk.modules.split import Split
from kmk.modules.layers import Layers
from kmk.modules.modtap import ModTap
from kmk.modules.holdtap import HoldTapRepeat
import scanner

keyboard = KMKKeyboard()

row_pins = (board.D4, board.D5, board.D6)
adc_port = board.A0
discharge_port = board.D1
mux_sels = (board.D10, board.D9, board.D8)
col_channels=[4, 5, 3, 0, 1, 2]

tap_time = 125

modtap = ModTap()
modtap.tap_time = tap_time
modtap.prefer_hold = False
keyboard.modules.append(modtap)

layer = Layers()
layer.tap_time = tap_time
layer.prefer_hold = True
keyboard.modules.append(layer)

split = Split(
    use_pio=True,
    data_pin=board.D7,
    uart_interval=5,
)
keyboard.modules.append(split)

keyboard.coord_mapping = [
     4,  3,  2,  1,  0,  23, 22, 21, 20, 19,
    10,  9,  8,  7,  6,  29, 28, 27, 26, 25,
    16, 15, 14, 13, 12,  35, 34, 33, 32, 31,
            17, 11,  5,  18, 24, 30
]

_lower = 1
_raise = 2
_adjust = 3

EscGUI = KC.MT(KC.ESC, KC.RGUI, repeat=HoldTapRepeat.HOLD)
LowMins = KC.LT(_lower, KC.MINS, repeat=HoldTapRepeat.HOLD)
AdjEnt = KC.LT(_adjust, KC.ENT, repeat=HoldTapRepeat.HOLD)
AdjMins = KC.LT(_adjust, KC.MINS, repeat=HoldTapRepeat.HOLD)
Adjust = KC.MO(_adjust)
RaiEnt = KC.LT(_raise, KC.ENT, prefer_hold=True, repeat=HoldTapRepeat.HOLD)
Sands = KC.MT(KC.SPC, KC.RSFT, repeat=HoldTapRepeat.HOLD)


keyboard.keymap = [
    # default layer
    [
        KC.QUOT, KC.COMM, KC.DOT , KC.P   , KC.Y   ,  KC.F   , KC.G   , KC.C   , KC.R   , KC.L   ,
        KC.A   , KC.O   , KC.E   , KC.U   , KC.I   ,  KC.D   , KC.H   , KC.T   , KC.N   , KC.S   ,
        KC.SCLN, KC.Q   , KC.J   , KC.K   , KC.X   ,  KC.B   , KC.M   , KC.W   , KC.V   , KC.Z   ,
                          EscGUI , LowMins, KC.LCTL,  RaiEnt , Sands  , KC.GRV
    ],
    # QWERTY
    # [
    #     KC.Q   , KC.W   , KC.E   , KC.R   , KC.T   ,  KC.Y   , KC.U   , KC.I   , KC.O   , KC.P   ,
    #     KC.A   , KC.S   , KC.D   , KC.F   , KC.G   ,  KC.H   , KC.J   , KC.K   , KC.L   , KC.SCLN,
    #     KC.Z   , KC.X   , KC.C   , KC.V   , KC.B   ,  KC.N   , KC.M   , KC.COMM, KC.DOT , KC.SLSH,
    #                       EscGUI , LowMins, KC.LCTL,  RaiEnt , Sands  , KC.NO
    # ],
    # lower
    [
        KC.EXLM, KC.AT  , KC.HASH, KC.DLR , KC.PERC,  KC.CIRC, KC.AMPR, KC.UP  , KC.LPRN , KC.RPRN,
        KC.CIRC, KC.AMPR, KC.LPRN, KC.LBRC, KC.LCBR,  KC.HOME, KC.LEFT, KC.DOWN, KC.RIGHT, KC.BSPC,
        KC.SCLN, KC.ASTR, KC.RPRN, KC.RBRC, KC.RCBR,  KC.END , KC.ENT , KC.LBRC, KC.RBRC , KC.DEL ,
                          KC.TRNS, KC.TRNS, KC.NO  ,  Adjust , KC.TRNS, KC.TRNS
    ],
    # raise
    [
        KC.N1  , KC.N2  , KC.N3  , KC.N4  , KC.N5  ,  KC.LBRC, KC.RBRC, KC.TAB , KC.SLSH, KC.QUES,
        KC.N6  , KC.N7  , KC.N8  , KC.N9  , KC.N0  ,  KC.PIPE, KC.BSLS, KC.GRV , KC.EQL , KC.RSFT,
        KC.LCTL, KC.LSFT, KC.J   , KC.K   , KC.X   ,  KC.ENT , KC.PLUS, KC.TILD, KC.QUOT, KC.RSFT,
                          KC.TRNS, Adjust , KC.NO  ,  KC.TRNS, KC.TRNS, KC.TRNS
    ],
    # adjust
    [
        KC.EXLM, KC.AT  , KC.HASH, KC.DLR , KC.PERC,  KC.CIRC, KC.AMPR, KC.ASTR, KC.LBRC, KC.RBRC,
        KC.NO  , KC.NO  , KC.NO  , KC.NO  , KC.NO  ,  KC.PIPE, KC.BSLS, KC.GRV , KC.LCBR, KC.RCBR,
        KC.F1  , KC.F2  , KC.F3  , KC.F4  , KC.F5  ,  KC.F6  , KC.F7  , KC.F8  , KC.F9  , KC.F10 ,
                          KC.TRNS, KC.TRNS, KC.NO  ,  KC.TRNS, KC.TRNS, KC.TRNS
    ],
]

keyboard.matrix = scanner.ECMatrixScanner(
    col_channels=col_channels,
    rows=row_pins,
    mux_sels=mux_sels,
    adc_port=adc_port,
    discharge_port=discharge_port,
    low_threshold=0.7,
    high_threshold=0.8,
    debug=False,
)

if __name__ == '__main__':
    keyboard.go()
