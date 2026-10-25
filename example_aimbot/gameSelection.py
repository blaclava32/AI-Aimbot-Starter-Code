"""Interactive game-window selection and capture-region setup."""

from __future__ import annotations

import time
from typing import Optional, Tuple

import bettercam
import pygetwindow

try:
    from .schema.settings import Settings
except ImportError:
    # Fallback when this package directory is placed directly on sys.path.
    from schema.settings import Settings


def gameSelection(
    settings: Settings,
) -> Optional[Tuple[bettercam.BetterCam, int, int]]:
    """Prompt for a window, activate it, and start a BetterCam capture region."""
    try:
        video_game_windows = pygetwindow.getAllWindows()
        print("=== All Windows ===")
        for index, window in enumerate(video_game_windows):
            if window.title:
                print(f"[{index}]: {window.title}")

        try:
            user_input = int(
                input(
                    "Please enter the number corresponding to the window you'd like to select: "
                )
            )
        except ValueError:
            print("You didn't enter a valid number. Please try again.")
            return None

        if user_input < 0 or user_input >= len(video_game_windows):
            print("Selected index is out of range.")
            return None

        video_game_window = video_game_windows[user_input]
    except Exception as exc:
        print(f"Failed to select game window: {exc}")
        return None

    activation_retries = 30
    activation_success = False
    while activation_retries > 0:
        try:
            video_game_window.activate()
            activation_success = True
            break
        except pygetwindow.PyGetWindowException as window_error:
            print(f"Failed to activate game window: {window_error}")
            print("Trying again... (you should switch to the game now)")
        except Exception as exc:
            print(f"Failed to activate game window: {exc}")
            print(
                "Read the relevant restrictions here: "
                "https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setforegroundwindow"
            )
            activation_success = False
            break

        time.sleep(3.0)
        activation_retries -= 1

    if not activation_success:
        return None

    print("Successfully activated the game window...")

    left = ((video_game_window.left + video_game_window.right) // 2) - (
        settings.screenShotWidth // 2
    )
    top = video_game_window.top + (
        (video_game_window.height - settings.screenShotHeight) // 2
    )
    right = left + settings.screenShotWidth
    bottom = top + settings.screenShotHeight
    region = (left, top, right, bottom)

    center_width = settings.screenShotWidth // 2
    center_height = settings.screenShotHeight // 2

    print(region)

    camera = bettercam.create(region=region, output_color="BGRA", max_buffer_len=512)
    if camera is None:
        print(
            "Your Camera Failed! Ask @Wonder for help in our Discord in the "
            "#ai-aimbot channel ONLY: https://discord.gg/rootkitorg"
        )
        return None

    camera.start(target_fps=120, video_mode=True)
    return camera, center_width, center_height
