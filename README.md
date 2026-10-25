> [!CAUTION]
> # This tool is No Longer Supported by RootKit
> We will not offer help for issues that arise with this program, or assist in getting it running on your machine.
> You are free to fork and edit this tool yourself.

![World's Best AI Aimbot Banner](imgs/banner.png)

[![Pull Requests Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat)](http://makeapullrequest.com)

# AI Aimbot Starter Code

Custom-code starter templates for the [RootKit Launcher](https://github.com/RootKit-Org/Launcher). The launcher loads your module and calls `main(...)` with model/settings arguments.

> **Note:** RootKit no longer supports this starter. Forks and self-help only.

## Requirements

- Windows (launcher paths use `%APPDATA%`; examples use `pywin32` / BetterCam)
- Python 3.10+ recommended
- Dependencies from `requirements.txt` (GPU/CUDA optional but recommended for the aimbot example)

```bash
pip install -r requirements.txt
```

Download the free [RootKit Launcher](https://github.com/RootKit-Org/Launcher) if you want store models, settings profiles, and packaging. No coding is required to use the launcher itself.

## What's in the box?

| Path | Purpose |
|------|---------|
| `main.py` | Local harness that mimics how the launcher invokes custom code |
| `example_bare/` | Minimal module: load settings + resolve model path |
| `example_aimbot/` | Full open-source aimbot example (PyTorch `.pt` models) |
| `imgs/` | README assets |

## Running locally

From the repository root:

```bash
python main.py <settingsProfile> <yoloVersion> <modelFileName> <customCode>
```

Example:

```bash
python main.py Default 5 v5_base_s.pt example_bare.main
```

| Argument | Meaning |
|----------|---------|
| `settingsProfile` | JSON name under `%APPDATA%\ai-aimbot-launcher\aimbotSettings\` (without `.json`) |
| `yoloVersion` | `5` or `8` |
| `modelFileName` | File under `%APPDATA%\ai-aimbot-launcher\models\` (`.pt`, `.onnx`, or `.engine`) |
| `customCode` | Dotted module path with a `main` function, e.g. `example_bare.main` |

Treat `customCode` like a Python import path relative to this repo.

## Deploying to the launcher

Copy your custom-code folder into:

`%APPDATA%\ai-aimbot-launcher\customCode`

To publish on the store, ping `@Technical Champions` in the Discord.

## Launcher → `main(...)` contract

The launcher calls your module’s `main` with:

```python
version: int          # 0–2 → pytorch, onnx, engine
settingsProfile: str  # settings file stem in aimbotSettings
paidTier: int         # 0–3 → free, supporter t1, t2, t3
yoloVersion: int      # 5 or 8
modelFileName: str    # model file in models/
```

### Example 1 — accept anything

```python
def main(**argv):
    print("My custom bot")
    print(argv)
```

### Example 2 — named parameters

```python
def main(
    version,
    settingsProfile,
    paidTier,
    yoloVersion,
    modelFileName,
):
    print("My custom bot")
```

### Example 3 — load settings + model path

```python
from .schema.settings import Settings
import json
import os

def main(
    version: int = 0,
    settingsProfile: str = "",
    paidTier: int = 0,
    yoloVersion: int = 0,
    modelFileName: str = "",
):
    appdata = os.environ["APPDATA"]
    settings_path = os.path.join(
        appdata,
        "ai-aimbot-launcher",
        "aimbotSettings",
        f"{settingsProfile.lower()}.json",
    )

    with open(settings_path, "r", encoding="utf-8") as handle:
        settings = Settings(**json.load(handle))

    model_path = os.path.join(
        appdata, "ai-aimbot-launcher", "models", modelFileName
    )
    print(settings.confidence, model_path)
```

## Example notes

- **`example_bare`**: safest place to start; prints loaded settings metadata.
- **`example_aimbot`**: Windows-only capture + mouse move loop; **PyTorch `.pt` only** in this starter. Hold the configured activation key (default Caps Lock) to move; press the quit key (default `Q`) to stop.
- Vendored YOLOv5 helpers under `example_aimbot/models` and `example_aimbot/utils` come from upstream Ultralytics-style sources and are kept for compatibility.

## Pros of using the starter + launcher

- Competitive / store submission baseline
- Access to launcher store models (auto-converted)
- Settings profiles available to your code
- Publish custom code on the store

## Cons

- You need to learn the launcher custom-code API

## Disclaimer

Using automation in online games may violate game terms of service and can result in bans. This repository is provided as unsupported starter code only.
