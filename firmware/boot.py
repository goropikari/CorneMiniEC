import board
import storage
import digitalio
import supervisor
import usb_cdc
import usb_hid
import os

supervisor.runtime.next_stack_limit = 4096 + 4096
supervisor.runtime.autoreload = False

if os.getenv('DEBUG') != 1:
    usb_cdc.disable()

# タクトスイッチを押しながらだとストレージとして認識される
reset = digitalio.DigitalInOut(board.D2)
reset.pull = digitalio.Pull.UP
if reset.value:
    storage.disable_usb_drive()
