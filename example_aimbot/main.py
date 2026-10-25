"""Open-source aimbot example wired for the RootKit launcher SDK."""

from __future__ import annotations

import json
import os
import time
from pathlib import Path

import cv2
import numpy as np
import pandas as pd
import torch
import win32api
import win32con

from .schema.settings import Settings
from .utils.general import non_max_suppression, xyxy2xywh

try:
    from . import gameSelection
except ImportError:
    import gameSelection


def _require_appdata() -> str:
    appdata = os.getenv("APPDATA")
    if not appdata:
        raise EnvironmentError(
            "APPDATA is not set. This starter targets the Windows launcher layout."
        )
    return appdata


def _load_settings(appdata: str, settings_profile: str) -> Settings:
    settings_path = Path(appdata) / "ai-aimbot-launcher" / "aimbotSettings" / (
        f"{settings_profile.lower()}.json"
    )
    if not settings_path.is_file():
        raise FileNotFoundError(f"Settings profile not found: {settings_path}")

    with settings_path.open("r", encoding="utf-8") as handle:
        payload = json.load(handle)
    return Settings(**payload)


def main(
    version: int = 0,
    settingsProfile: str = "",
    paidTier: int = 0,
    yoloVersion: int = 0,
    modelFileName: str = "",
) -> None:
    """Entrypoint invoked by the launcher / local ``main.py`` harness."""
    del version, paidTier, yoloVersion  # Provided by the launcher contract.

    appdata = _require_appdata()
    settings = _load_settings(appdata, settingsProfile)
    model_path = Path(appdata) / "ai-aimbot-launcher" / "models" / modelFileName

    if model_path.suffix.lower() != ".pt":
        print("Fast Mode ONLY! (PyTorch)")
        return

    run_bot(str(model_path), settings)


def run_bot(model_path: str, settings: Settings) -> None:
    selection = gameSelection.gameSelection(settings)
    if selection is None:
        print("Game selection failed; exiting.")
        return

    camera, center_width, center_height = selection

    cps_count = 0
    sample_started_at = time.time()

    model = torch.hub.load("ultralytics/yolov5", "custom", path=model_path)
    names = model.names

    if torch.cuda.is_available():
        model.half()

    colors = np.random.uniform(0, 255, size=(1500, 3))
    last_mid_coord = None
    body_offset_factor = 0.2

    try:
        with torch.no_grad():
            while win32api.GetAsyncKeyState(settings.quitKey) == 0:
                np_img = np.array(camera.get_latest_frame())

                if settings.useMask:
                    if settings.maskLeft:
                        np_img[-settings.maskHeight :, : settings.maskWidth, :] = 0
                    else:
                        np_img[-settings.maskHeight :, -settings.maskWidth :, :] = 0

                image = torch.from_numpy(np_img)
                if image.shape[2] == 4:
                    image = image[:, :, :3]

                image = torch.movedim(image, 2, 0)
                if torch.cuda.is_available():
                    image = image.half()
                    image /= 255
                if len(image.shape) == 3:
                    image = image[None]

                results = model(image, size=settings.screenShotHeight)
                predictions = non_max_suppression(
                    results,
                    settings.confidence,
                    settings.confidence,
                    0,
                    False,
                    max_det=1000,
                )

                targets = []
                for det in predictions:
                    if not len(det):
                        continue

                    gain = torch.tensor(image.shape)[[0, 0, 0, 0]]
                    for *xyxy, conf, _cls in reversed(det):
                        targets.append(
                            (
                                xyxy2xywh(torch.tensor(xyxy).view(1, 4)) / gain
                            )
                            .view(-1)
                            .tolist()
                            + [float(conf)]
                        )

                targets_df = pd.DataFrame(
                    targets,
                    columns=[
                        "current_mid_x",
                        "current_mid_y",
                        "width",
                        "height",
                        "confidence",
                    ],
                )

                if len(targets_df) > 0:
                    if settings.centerOfScreen:
                        targets_df["dist_from_center"] = np.sqrt(
                            (targets_df.current_mid_x - center_width) ** 2
                            + (targets_df.current_mid_y - center_height) ** 2
                        )
                        targets_df = targets_df.sort_values("dist_from_center")

                    if last_mid_coord is not None:
                        # Track continuity distance for debugging / future use.
                        # Historically sort_values(...) was not assigned, so
                        # selection continued to use center-of-screen order above.
                        targets_df["last_mid_x"] = last_mid_coord[0]
                        targets_df["last_mid_y"] = last_mid_coord[1]
                        targets_df["dist"] = np.linalg.norm(
                            targets_df[["current_mid_x", "current_mid_y"]].values
                            - targets_df[["last_mid_x", "last_mid_y"]].values,
                            axis=1,
                        )

                    x_mid = targets_df.iloc[0].current_mid_x
                    y_mid = targets_df.iloc[0].current_mid_y
                    box_height = targets_df.iloc[0].height

                    if settings.headshotMode:
                        headshot_offset = box_height * settings.headshotDistanceModifier
                    else:
                        headshot_offset = box_height * body_offset_factor

                    mouse_move = [
                        x_mid - center_width,
                        (y_mid - headshot_offset) - center_height,
                    ]

                    if win32api.GetKeyState(settings.activationKey):
                        win32api.mouse_event(
                            win32con.MOUSEEVENTF_MOVE,
                            int(mouse_move[0] * settings.movementAmp),
                            int(mouse_move[1] * settings.movementAmp),
                            0,
                            0,
                        )
                    last_mid_coord = [x_mid, y_mid]
                else:
                    last_mid_coord = None

                if settings.visuals:
                    for index in range(len(targets_df)):
                        half_w = round(targets_df["width"].iloc[index] / 2)
                        half_h = round(targets_df["height"].iloc[index] / 2)
                        mid_x = targets_df["current_mid_x"].iloc[index]
                        mid_y = targets_df["current_mid_y"].iloc[index]
                        start_x = int(mid_x + half_w)
                        start_y = int(mid_y + half_h)
                        end_x = int(mid_x - half_w)
                        end_y = int(mid_y - half_h)

                        label = "{}: {:.2f}%".format(
                            names.get(0, "Human")
                            if isinstance(names, dict)
                            else "Human",
                            targets_df["confidence"].iloc[index] * 100,
                        )
                        color = colors[0]
                        cv2.rectangle(np_img, (start_x, start_y), (end_x, end_y), color, 2)
                        text_y = start_y - 15 if start_y - 15 > 15 else start_y + 15
                        cv2.putText(
                            np_img,
                            label,
                            (start_x, text_y),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            0.5,
                            color,
                            2,
                        )

                cps_count += 1
                if (time.time() - sample_started_at) > 1:
                    if settings.displayCPS:
                        print(f"CPS: {cps_count}")
                    cps_count = 0
                    sample_started_at = time.time()

                if settings.visuals:
                    cv2.imshow("Live Feed", np_img)
                    if (cv2.waitKey(1) & 0xFF) == ord("q"):
                        break
    finally:
        camera.stop()
        if settings.visuals:
            cv2.destroyAllWindows()


# Keep the historical name available for forks / docs that call Bot(...).
Bot = run_bot
