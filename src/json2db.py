# @file      json2db.py
# 
# @version   0.1.0
#
# @date      17-09-2026
#  
# @author    Fábio D. Pacheco, 
# @email     fabio.d.pacheco@inesctec.pt or pacheco.castro.fabio@gmail.com
# 
# @note
# sibdb 
# Copyright (C) 2026 Fábio D. Pacheco 
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA

import subprocess

BIN_PATH = "/usr/local/bin/json2dbcli.exe"
def json2db_add(tabname, timestamp, json_payload):
        cmd = [str(BIN_PATH), "--add", tabname, timestamp, json_payload]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
                raise RuntimeError(f"[json2dbcli.exe][ERROR ] {res.stderr}")
        return res.stdout