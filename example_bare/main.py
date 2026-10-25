"""Minimal custom-code example for the RootKit launcher SDK."""

from __future__ import annotations

import json
import os
from pathlib import Path

from pydantic import ValidationError

from .schema.settings import Settings


def _require_appdata() -> str:
    appdata = os.getenv("APPDATA")
    if not appdata:
        raise OSError(
            "APPDATA is not set. This starter targets the Windows launcher layout."
        )
    return appdata


def _plain_name(value: str, *, label: str) -> str:
    """Reject empty names and path-like values for launcher profile/model fields."""
    name = (value or "").strip()
    if not name:
        raise ValueError(f"{label} is empty")
    if any(sep in name for sep in ("/", "\\")) or name in {".", ".."} or ".." in Path(name).parts:
        raise ValueError(
            f"Invalid {label} {value!r}: use a plain file/profile name, not a path"
        )
    return name


def _load_settings(appdata: str, settings_profile: str) -> Settings:
    profile = _plain_name(settings_profile, label="settingsProfile")
    settings_path = (
        Path(appdata) / "ai-aimbot-launcher" / "aimbotSettings" / f"{profile.lower()}.json"
    )
    if not settings_path.is_file():
        raise FileNotFoundError(f"Settings profile not found: {settings_path}")

    try:
        with settings_path.open("r", encoding="utf-8") as handle:
            payload = json.load(handle)
    except json.JSONDecodeError as err:
        raise ValueError(
            f"Settings profile is not valid JSON ({settings_path}): {err}"
        ) from err

    if not isinstance(payload, dict):
        raise TypeError(f"Settings profile must be a JSON object: {settings_path}")

    try:
        return Settings(**payload)
    except ValidationError as err:
        raise ValueError(f"Invalid settings in {settings_path}:\n{err}") from err


def _resolve_model_path(appdata: str, model_file_name: str) -> Path:
    name = _plain_name(model_file_name, label="modelFileName")
    return Path(appdata) / "ai-aimbot-launcher" / "models" / name


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
    model_path = _resolve_model_path(appdata, modelFileName)

    print("BASE CODE")
    print(
        f"version={version} paidTier={paidTier} yoloVersion={yoloVersion} "
        f"model={model_path} confidence={settings.confidence}"
    )
