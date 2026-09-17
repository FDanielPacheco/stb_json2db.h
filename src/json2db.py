import subprocess

BIN_PATH = "/usr/sbin/json2dbcli.exe"
def json2db_add(tabname, timestamp, json_payload):
        cmd = [str(BIN_PATH), "--add", tabname, json_payload]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
                raise RuntimeError(f"[json2dbcli.exe][ERROR ] {res.stderr}")
        return res.stdout