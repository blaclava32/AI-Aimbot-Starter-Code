"""Local test harness for RootKit launcher custom-code modules.

Usage:
    python main.py <settingsProfile> <yoloVersion> <modelFileName> <customCode>

Example:
    python main.py Default 5 v5_base_s.pt example_bare.main
"""

from __future__ import annotations

import argparse
import importlib
import os
import sys
import time
import traceback
from pathlib import Path
from types import ModuleType

from colorama import Fore, Style
from colorama import init as colorama_init

colorama_init(autoreset=True)

# When True, simulate a paid-tier unlock for local testing.
TEST = True

REPO_ROOT = Path(__file__).resolve().parent


def _print(color: str, message: object) -> None:
    print(f"{color}{message}{Style.RESET_ALL}")


def pr_red(message: object) -> None:
    _print(Fore.RED, message)


def pr_green(message: object) -> None:
    _print(Fore.GREEN, message)


def pr_yellow(message: object) -> None:
    _print(Fore.YELLOW, message)


def pr_blue(message: object) -> None:
    _print(Fore.BLUE, message)


def _extend_windows_path() -> None:
    """Append common Windows CUDA / Python paths when present (no-op elsewhere)."""
    if os.name != "nt":
        return

    local_appdata = os.getenv("LOCALAPPDATA") or ""
    path_additions = [
        rf"{local_appdata}\Programs\Python\Python311\Scripts",
        rf"{local_appdata}\Programs\Python\Python311",
        r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8",
        r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\bin",
        r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\libnvvp",
        r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8\lib",
    ]

    current_path = os.environ.get("Path") or os.environ.get("PATH") or ""
    for entry in path_additions:
        if entry and entry not in current_path and Path(entry).exists():
            current_path = f"{current_path};{entry}"

    os.environ["Path"] = current_path
    os.environ["PATH"] = current_path

    cuda_root = Path(r"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.8")
    if cuda_root.exists():
        os.environ.setdefault("CUDA_PATH", str(cuda_root))
        os.environ.setdefault("CUDA_PATH_V11_8", str(cuda_root))


def _resolve_model_backend(model_file_name: str) -> tuple[int, str]:
    extension = Path(model_file_name).suffix.lower()
    backends = {
        ".pt": (0, "Fast (PyTorch)"),
        ".onnx": (1, "Faster (ONNX)"),
        ".engine": (2, "Fastest (TensorRT)"),
    }
    if extension not in backends:
        raise ValueError(
            "Invalid model file extension. Please use .pt, .onnx, or .engine "
            f"(got {model_file_name!r})"
        )
    return backends[extension]


def _load_custom_code(module_path: str) -> ModuleType:
    """Import a dotted module path such as ``example_bare.main``."""
    if not module_path or not module_path.strip():
        raise ImportError("customCode module path is empty")

    if str(REPO_ROOT) not in sys.path:
        sys.path.insert(0, str(REPO_ROOT))

    # Preserve legacy behavior: also add the top-level package directory so
    # custom code that uses absolute sibling imports (e.g. ``import gameSelection``)
    # continues to work.
    package_name = module_path.split(".", 1)[0]
    package_dir = REPO_ROOT / package_name
    if package_dir.is_dir() and str(package_dir) not in sys.path:
        sys.path.insert(0, str(package_dir))

    try:
        module = importlib.import_module(module_path)
    except Exception as err:
        raise ImportError(f"Failed to import custom code '{module_path}': {err}") from err

    if not callable(getattr(module, "main", None)):
        raise ImportError(
            f"Custom code '{module_path}' must expose a callable main(...)"
        )
    return module


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run a launcher-compatible custom code module locally.",
    )
    parser.add_argument("settings_profile", help="Settings profile name (without .json)")
    parser.add_argument(
        "yolo_version",
        type=int,
        choices=(5, 8),
        help="YOLO major version (5 or 8)",
    )
    parser.add_argument("model_file_name", help="Model filename (.pt, .onnx, or .engine)")
    parser.add_argument(
        "custom_code",
        nargs="?",
        default=None,
        help="Dotted module path exposing main(), e.g. example_bare.main",
    )
    parser.add_argument(
        "--no-countdown",
        action="store_true",
        help="Skip the exit countdown (useful for scripts / CI)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    _extend_windows_path()

    try:
        args = parse_args(argv)
    except SystemExit as exc:
        # argparse already printed help / errors
        return int(exc.code) if isinstance(exc.code, int) else 1

    try:
        bot_version, version_text = _resolve_model_backend(args.model_file_name)
    except ValueError as err:
        pr_red(err)
        return 1

    paid_tier = 1 if TEST else 0

    pr_green(f"Version: {version_text} YOLOv{args.yolo_version} from {args.model_file_name}")
    pr_green(f"Settings Profile: {args.settings_profile}")

    if args.custom_code:
        try:
            custom_code = _load_custom_code(args.custom_code)
        except ImportError as err:
            pr_red(err)
            return 1

        pr_blue(f"Custom Code: {args.custom_code}")
        custom_code.main(
            version=bot_version,
            settingsProfile=args.settings_profile,
            paidTier=paid_tier,
            yoloVersion=args.yolo_version,
            modelFileName=args.model_file_name,
        )
    else:
        pr_blue("RUNNING MAIN BOT")
        pr_yellow("No custom code module provided; nothing else to run.")

    return 0


if __name__ == "__main__":
    exit_code = 0
    skip_countdown = "--no-countdown" in sys.argv
    try:
        exit_code = main()
    except Exception as exc:
        pr_red(traceback.format_exc())
        pr_red(exc)
        pr_yellow(
            "Ask @Wonder for help in our Discord: https://discord.gg/rootkitorg"
        )
        exit_code = 1

    if not skip_countdown:
        for remaining in range(3, 0, -1):
            pr_yellow(f"Bot will close in {remaining} seconds")
            time.sleep(1)

    raise SystemExit(exit_code)
