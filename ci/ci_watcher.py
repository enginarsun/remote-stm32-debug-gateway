#!/usr/bin/env python3
"""
Simple polling-based CI for the remote STM32 debug gateway.
Checks GitHub for new commits every POLL_INTERVAL seconds.
When a new commit is found: pulls it, rebuilds firmware, flashes it,
and checks UART output for expected sensor data.

No inbound connections are accepted -- this only makes outbound
requests to GitHub, so there is nothing to attack from outside.

Set your own GitHub Personal Access Token (repo scope) below, or
export it as an environment variable and read it via os.environ
instead of hardcoding it.
"""

import subprocess
import time
import json
import urllib.request
from datetime import datetime

REPO_API_URL = "https://api.github.com/repos/enginarsun/remote-stm32-debug-gateway/commits/main"
REPO_DIR = "/home/engin/ci-repo"
POLL_INTERVAL = 60
LOG_FILE = "/home/engin/ci-repo/ci_log.txt"
GITHUB_TOKEN = "PASTE_YOUR_TOKEN_HERE"

last_sha = None


def log(message):
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{timestamp}] {message}"
    print(line)
    try:
        with open(LOG_FILE, "a") as f:
            f.write(line + "\n")
    except Exception:
        pass


def get_latest_sha():
    try:
        req = urllib.request.Request(
            REPO_API_URL,
            headers={
                "User-Agent": "pi-ci-watcher",
                "Authorization": f"token {GITHUB_TOKEN}",
                "Accept": "application/vnd.github+json"
            }
        )
        with urllib.request.urlopen(req, timeout=10) as response:
            data = json.loads(response.read().decode())
            return data["sha"]
    except Exception as e:
        log(f"ERROR checking GitHub: {e}")
        return None


def run_cmd(cmd, cwd=None, timeout=60):
    result = subprocess.run(
        cmd, shell=True, cwd=cwd,
        capture_output=True, text=True, timeout=timeout
    )
    return result.returncode, result.stdout + result.stderr


def read_uart_for_seconds(seconds=8):
    import serial
    try:
        ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1, dsrdtr=False, rtscts=False)
        end_time = time.time() + seconds
        buffer = ""
        while time.time() < end_time:
            chunk = ser.read(64).decode(errors='ignore')
            buffer += chunk
        ser.close()
        return buffer
    except Exception as e:
        return f"UART_ERROR: {e}"


def run_pipeline():
    log("=== New commit detected, starting pipeline ===")

    code, out = run_cmd("git pull", cwd=REPO_DIR)
    log(f"git pull: exit={code}")
    if code != 0:
        log(f"FAILED at git pull:\n{out}")
        return False

    firmware_dir = f"{REPO_DIR}/firmware"
    code, out = run_cmd(
        "arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -nostdlib -nostartfiles "
        "-T stm32f103.ld -o blink.elf startup.c main.c",
        cwd=firmware_dir
    )
    log(f"compile: exit={code}")
    if code != 0:
        log(f"FAILED at compile:\n{out}")
        return False

    code, out = run_cmd(
        "arm-none-eabi-objcopy -O binary blink.elf blink.bin",
        cwd=firmware_dir
    )
    if code != 0:
        log(f"FAILED at objcopy:\n{out}")
        return False

    code, out = run_cmd(
        f"st-flash --connect-under-reset write {firmware_dir}/blink.bin 0x08000000",
        timeout=30
    )
    log(f"flash: exit={code}")
    if "Flash written and verified" not in out:
        log(f"FAILED at flash:\n{out}")
        return False

    log("Flash successful, reading UART for verification...")
    time.sleep(2)
    uart_output = read_uart_for_seconds(8)

    if "Sicaklik=" in uart_output and "Nem=" in uart_output:
        log(f"PASS: sensor output detected\n{uart_output[-200:]}")
        return True
    else:
        log(f"FAIL: expected sensor output not found\n{uart_output[-200:]}")
        return False


def main():
    global last_sha
    log("CI watcher started. Polling every 60s. No inbound connections accepted.")

    last_sha = get_latest_sha()
    log(f"Initial commit: {last_sha}")

    while True:
        time.sleep(POLL_INTERVAL)
        current_sha = get_latest_sha()

        if current_sha and current_sha != last_sha:
            log(f"New commit found: {current_sha}")
            last_sha = current_sha
            success = run_pipeline()
            log(f"=== Pipeline {'PASSED' if success else 'FAILED'} ===\n")


if __name__ == "__main__":
    main()
