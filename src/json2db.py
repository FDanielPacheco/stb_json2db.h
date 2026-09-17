import subprocess
from pathlib import Path

BIN_PATH = Path(__file__).parent / ".." / "build" / "json2dbcli.exe"
def json2db_add(tabname, timestamp, json_payload):
        cmd = [str(BIN_PATH), "--add", tabname, json_payload]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
                raise RuntimeError(f"CLI Error: {res.stderr}")
        return res.stdout

json2db_add(
        "gps", "now",
        "{\"lat\": 41.1579, \"lon\": -8.6291, \"altitude\": 150.2, \"satellites\": 9, \"fix\": true}"
)