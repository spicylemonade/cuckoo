import subprocess
import sys
from pathlib import Path

# Prefer importing function if present
try:
    from hello import hello
    def test_hello_function():
        assert hello() == "hello world"
except Exception:
    # Fallback: run the script as a program
    def test_hello_script_execution():
        py = sys.executable
        script = str(Path(__file__).resolve().parents[1] / 'hello.py')
        result = subprocess.run([py, script], capture_output=True, text=True)
        assert result.returncode == 0
        assert result.stdout.strip() == 'hello world'
