import subprocess
import os

def test_esphome_config():
    # Run from the root directory
    result = subprocess.run(["esphome", "config", "tests/python/test_config.yaml"], capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr)
    assert result.returncode == 0
