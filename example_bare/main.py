"""Minimal custom-code example for the RootKit launcher SDK."""

from __future__ import annotations

import json
import os
from pathlib import Path

from .schema.settings import Settings


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
    appdata = _require_appdata()
    settings = _load_settings(appdata, settingsProfile)
    model_path = Path(appdata) / "ai-aimbot-launcher" / "models" / modelFileName

    print("BASE CODE")
    print(
        f"version={version} paidTier={paidTier} yoloVersion={yoloVersion} "
        f"model={model_path} confidence={settings.confidence}"
    )
