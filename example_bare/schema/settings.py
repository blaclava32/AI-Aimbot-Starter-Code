"""Pydantic settings schema matching launcher aimbotSettings JSON profiles.

Virtual-key codes match ``win32con`` values so this module imports without
``pywin32`` (useful for editing / validating settings outside Windows).
"""

from __future__ import annotations

from typing import Any, Optional

from pydantic import BaseModel, ConfigDict, Field, field_validator

# https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
VK_LBUTTON = 0x01
VK_RBUTTON = 0x02
VK_MBUTTON = 0x04
VK_BACK = 0x08
VK_TAB = 0x09
VK_RETURN = 0x0D
VK_SHIFT = 0x10
VK_CONTROL = 0x11
VK_MENU = 0x12
VK_CAPITAL = 0x14
VK_ESCAPE = 0x1B
VK_PRIOR = 0x21
VK_NEXT = 0x22
VK_END = 0x23
VK_HOME = 0x24
VK_INSERT = 0x2D
VK_DELETE = 0x2E
VK_LSHIFT = 0xA0
VK_RSHIFT = 0xA1
VK_LCONTROL = 0xA2
VK_RCONTROL = 0xA3
VK_LMENU = 0xA4
VK_RMENU = 0xA5
VK_F1 = 0x70
VK_F2 = 0x71
VK_F3 = 0x72
VK_F4 = 0x73
VK_F5 = 0x74
VK_F6 = 0x75
VK_F7 = 0x76
VK_F8 = 0x77
VK_F9 = 0x78
VK_F10 = 0x79
VK_F11 = 0x7A
VK_F12 = 0x7B

KEY_MAP = {
    "F1": VK_F1,
    "F2": VK_F2,
    "F3": VK_F3,
    "F4": VK_F4,
    "F5": VK_F5,
    "F6": VK_F6,
    "F7": VK_F7,
    "F8": VK_F8,
    "F9": VK_F9,
    "F10": VK_F10,
    "F11": VK_F11,
    "F12": VK_F12,
    "Escape": VK_ESCAPE,
    "Tab": VK_TAB,
    "CapsLock": VK_CAPITAL,
    "LeftShift": VK_LSHIFT,
    "Shift": VK_LSHIFT,
    "RightShift": VK_RSHIFT,
    "LeftControl": VK_LCONTROL,
    "Control": VK_LCONTROL,
    "RightControl": VK_RCONTROL,
    "LeftAlt": VK_LMENU,
    "Alt": VK_LMENU,
    "RightAlt": VK_RMENU,
    "Enter": VK_RETURN,
    "Backspace": VK_BACK,
    "Delete": VK_DELETE,
    "Insert": VK_INSERT,
    "Home": VK_HOME,
    "End": VK_END,
    "PageUp": VK_PRIOR,
    "PageDown": VK_NEXT,
    "LeftMouseButtonDown": VK_LBUTTON,
    "RightMouseButtonDown": VK_RBUTTON,
    "MiddleMouseButtonDown": VK_MBUTTON,
}


class Settings(BaseModel):
    model_config = ConfigDict(extra="allow")

    movementAmp: float = Field(default=0.3)
    useMask: bool = Field(default=False)
    maskLeft: bool = Field(default=True)
    maskWidth: int = Field(default=80)
    maskHeight: int = Field(default=200)
    quitKey: int = Field(default=ord("Q"))
    screenShotHeight: int = Field(default=320)
    screenShotWidth: int = Field(default=320)
    confidence: float = Field(default=0.5)
    headshotMode: bool = Field(default=True)
    headshotDistanceModifier: float = Field(default=0.38)
    displayCPS: bool = Field(default=True)
    visuals: bool = Field(default=False)
    centerOfScreen: bool = Field(default=True)
    activationKey: int = Field(default=VK_CAPITAL)
    autoFire: bool = Field(default=False)
    autoFireActivationDistance: int = Field(default=50)
    onnxChoice: int = Field(default=2)
    fovCircle: bool = Field(default=False)
    fovCircleRadius: int = Field(default=160)
    fovCircleRadiusDetectionModifier: float = Field(default=1.0)
    aimShakey: bool = Field(default=False)
    aimShakeyStrength: int = Field(default=10)
    toggleable: bool = Field(default=True)
    gameTitle: Optional[str] = Field(default=None)

    @field_validator("activationKey", "quitKey", mode="before")
    @classmethod
    def map_key(cls, key: Any) -> int:
        if isinstance(key, int):
            return key

        if not isinstance(key, str):
            raise TypeError(f"Key must be str or int, got {type(key).__name__}")

        if len(key) == 1:
            try:
                return ord(key.upper())
            except Exception:
                print("Invalid activation key; defaulting to CapsLock")
                return KEY_MAP["CapsLock"]

        try:
            return KEY_MAP[key]
        except KeyError as err:
            raise ValueError(f"Unknown key name: {key!r}") from err
