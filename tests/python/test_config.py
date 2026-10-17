import subprocess
import os

def test_esphome_config():
    # Run a dry-run compile to trigger full code generation (to_code)
    result = subprocess.run(
        ["esphome", "compile", "--no-binary", "tests/python/test_config.yaml"], 
        capture_output=True, 
        text=True
    )
    if result.returncode != 0:
        print("\nESPHome Output:\n", result.stdout)
        print("\nESPHome Error:\n", result.stderr)
    assert result.returncode == 0, f"ESPHome config/compile failed with error: {result.stderr}"
