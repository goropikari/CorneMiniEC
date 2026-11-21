"""Emit the browser layout from the QMK keyboard definition."""

import json
from pathlib import Path

directory = Path(__file__).resolve().parent
keyboard = json.loads((directory.parent / "keyboard.json").read_text(encoding="utf-8"))
layout = keyboard["layouts"]["LAYOUT"]["layout"]
print(json.dumps(layout, ensure_ascii=False))
