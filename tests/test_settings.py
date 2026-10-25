"""Cross-platform unit tests for first-party settings schemas."""

from __future__ import annotations

import unittest

from pydantic import ValidationError

from example_aimbot.schema.settings import KEY_MAP as AIMBOT_KEYS
from example_aimbot.schema.settings import VK_CAPITAL as AIMBOT_CAPS
from example_aimbot.schema.settings import Settings as AimbotSettings
from example_bare.schema.settings import KEY_MAP as BARE_KEYS
from example_bare.schema.settings import VK_CAPITAL as BARE_CAPS
from example_bare.schema.settings import Settings as BareSettings


class TestSettingsSchema(unittest.TestCase):
    def test_defaults(self) -> None:
        settings = BareSettings()
        self.assertEqual(settings.confidence, 0.5)
        self.assertEqual(settings.activationKey, BARE_CAPS)
        self.assertEqual(settings.quitKey, ord("Q"))

    def test_key_name_and_char(self) -> None:
        settings = BareSettings(activationKey="F1", quitKey="q")
        self.assertEqual(settings.activationKey, BARE_KEYS["F1"])
        self.assertEqual(settings.quitKey, ord("Q"))

    def test_unknown_key_raises(self) -> None:
        with self.assertRaises(ValidationError):
            BareSettings(activationKey="NotARealKey")

    def test_empty_key_raises(self) -> None:
        with self.assertRaises(ValidationError):
            BareSettings(quitKey="")

    def test_confidence_bounds(self) -> None:
        with self.assertRaises(ValidationError):
            BareSettings(confidence=1.5)
        with self.assertRaises(ValidationError):
            BareSettings(confidence=-0.1)

    def test_screenshot_dims_positive(self) -> None:
        with self.assertRaises(ValidationError):
            BareSettings(screenShotWidth=0)

    def test_schemas_stay_in_sync(self) -> None:
        self.assertEqual(set(BareSettings.model_fields), set(AimbotSettings.model_fields))
        self.assertEqual(BARE_KEYS, AIMBOT_KEYS)
        self.assertEqual(BARE_CAPS, AIMBOT_CAPS)


class TestHarnessHelpers(unittest.TestCase):
    def test_resolve_model_backend(self) -> None:
        from main import _resolve_model_backend

        self.assertEqual(_resolve_model_backend("x.pt")[0], 0)
        self.assertEqual(_resolve_model_backend("x.onnx")[0], 1)
        self.assertEqual(_resolve_model_backend("x.engine")[0], 2)
        with self.assertRaises(ValueError):
            _resolve_model_backend("x.bin")

    def test_load_custom_code_requires_main(self) -> None:
        from main import _load_custom_code

        with self.assertRaises(ImportError):
            _load_custom_code("example_bare.schema.settings")


if __name__ == "__main__":
    unittest.main()
