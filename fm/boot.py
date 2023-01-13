import board
import storage
import digitalio
import supervisor
import usb_cdc
import usb_hid

supervisor.set_next_stack_limit(4096 + 4096)
supervisor.disable_autoreload()

# reset ボタン押しながらだとストレージとして認識される
reset = digitalio.DigitalInOut(board.D2)
reset.pull = digitalio.Pull.UP
if reset.value:
    storage.disable_usb_drive()

usb_cdc.disable()
usb_hid.enable(boot_device=1)
