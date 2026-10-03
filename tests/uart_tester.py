#!/usr/bin/env python3
"""Tester using UART to send console commands and verify responses."""

import configparser
import logging
import platform
import subprocess
from pathlib import Path
from typing import Final

from colorama import just_fix_windows_console
from serial import Serial, SerialException
from serial.tools import list_ports

from colors import cyan, green, red, yellow
from flash_tool import FlashTool
from subprocess_runner import SubprocessRunner
from test_loader import load_test_case
from test_runner import run_test
from test_types import TestCase, TestResult


_logger: logging.Logger

_platform_name: Final[str] = platform.system()

just_fix_windows_console()


def _start_echo_server(board_config: configparser.ConfigParser,
                       prefix: str) -> subprocess.Popen[bytes] | None:
    """Start the echo server executable."""
    enabled: bool = board_config["ECHO_SERVER"].getboolean("enabled",
                                                           fallback=False)
    if not enabled:
        return None

    executable: Path = Path(board_config["ECHO_SERVER"][f"{prefix}_path"])
    echo_command: list[str] = [executable.as_posix(),  "/p", "tcp", "/s", "7"]
    _logger.info("%s",
                 cyan(f"Starting echo server: {echo_command}"))

    process: subprocess.Popen[bytes] = SubprocessRunner(_logger).start(echo_command,
                                                                       stdout=subprocess.DEVNULL,
                                                                       stderr=subprocess.DEVNULL)

    return process


def _should_run_test(test: TestCase,
                     firmware_name: str) -> bool:
    """Check whether test applies."""
    if not test.firmware_concerned:
        return True

    return firmware_name in test.firmware_concerned


def _firmware_cli_path(firmware_file: Path,
                       subprocess_runner: SubprocessRunner) -> str:
    """Return the firmware path in the format expected by the flash CLI."""
    if not _platform_name.startswith("CYGWIN_NT"):
        return firmware_file.as_posix()

    result = subprocess_runner.run(["cygpath", "-w", firmware_file.as_posix()],
                                   "Unable to convert firmware path for STM32CubeProgrammer.")
    windows_path = result.stdout.strip()

    if not windows_path:
        raise RuntimeError(f"cygpath returned an empty path for {firmware_file}")

    return windows_path


def flash_firmware(firmware_file: Path,
                   address: str,
                   flash_tool: FlashTool,
                   board_config: configparser.ConfigParser,
                   subprocess_runner: SubprocessRunner) -> None:
    """Flash firmware when enabled in the board configuration."""
    enabled = board_config["FLASH"].getboolean("enabled",
                                               fallback=True)

    if not enabled:
        _logger.info(yellow("Flashing disabled"))
        return

    if not firmware_file.is_file():
        raise FileNotFoundError(red(f"Firmware not found: {firmware_file}"))

    firmware_cli_file = _firmware_cli_path(firmware_file, subprocess_runner)
    _logger.info(cyan(f"Flashing {firmware_cli_file}"))

    try:
        flash_tool.flash(firmware=firmware_cli_file,
                         address=address)
    except RuntimeError as error:
        _logger.error(red(f"Failed to flash {firmware_cli_file}: {error}"))
        raise


def _print_result(result: TestResult) -> None:
    """."""
    print("\n=============================================")
    print(result.name)
    print("=============================================\n")

    print(result.actual)

    if result.passed:
        print(green("PASS"))

    else:
        print(red("FAIL"))

        missing_items = [item.strip() for item in result.missing.split(",") if item.strip()]

        for item in missing_items:
            print(red(f"Missing: {item}"))


def main() -> None:
    """Main entry point for the firmware test runner."""
    global _logger

    board_config = configparser.ConfigParser()
    board_config.read("board.ini")

    logging_level: Final[logging._Level] = getattr(logging,
                                           board_config["LOG"].get("level", "INFO").upper())
    logging.basicConfig(level=logging_level,
                        format="%(asctime)s %(levelname)-8s %(message)s")

    _logger = logging.getLogger(__name__)
    subprocess_runner = SubprocessRunner(_logger)

    test_config = configparser.ConfigParser()
    test_config.read("test.ini")

    _logger.debug(cyan(_platform_name))

    prefix: str

    if _platform_name.startswith("Windows"):
        prefix = "windows"
    elif _platform_name.startswith("CYGWIN_NT"):
        prefix = "cygwin"
    else:
        prefix = "linux"

    port: Final[str] = board_config["UART"][f"{prefix}_port"]
    baudrate: Final[int] = int(board_config["UART"]["baudrate"])
    timeout_s: Final[float] = float(board_config["UART"]["timeout"])

    total: int = 0
    passed: int = 0
    echo_server_proc: subprocess.Popen[bytes] | None = None

    try:
        tool_programmer: Path=Path(board_config["TOOLS"][f"{prefix}_programmer"]).resolve()
        echo_server_proc = _start_echo_server(board_config, prefix)

        firmware_name: str
        for firmware_name in test_config.sections():

            if not firmware_name.startswith("FIRMWARE_"):
                continue

            firmware_file: Path = Path(test_config[firmware_name]["file"]).resolve()
            firmware_address: str = test_config[firmware_name].get("address", "0x08000000")
            firmware_map: Path = Path(test_config[firmware_name]["map_file"]).resolve()

            flash_tool: FlashTool = FlashTool(programmer=tool_programmer,
                                              serial_number=board_config["STLINK"]["serial_number"],
                                              map_file=firmware_map,
                                              logger=_logger)

            flash_firmware(firmware_file=firmware_file,
                           address=firmware_address,
                           flash_tool=flash_tool,
                           board_config=board_config,
                           subprocess_runner=subprocess_runner)

            # time.sleep(3.0)

            _logger.info("%s",
                         cyan(f"Opening {port} at {baudrate} baud"))

            with Serial(port=port,
                        baudrate=baudrate,
                        timeout=0.5) as uart:

                for section_name in test_config.sections():
                    if not section_name.startswith("TEST_"):
                        continue

                    test_case: TestCase = load_test_case(test_config,
                                                         section_name)

                    if not _should_run_test(test_case, firmware_name):
                        _logger.info(yellow(f"Skipping {test_case.name}"))
                        continue

                    result: TestResult = run_test(uart=uart,
                                                  test=test_case,
                                                  flash_tool=flash_tool,
                                                  timeout_s=timeout_s)
                    _print_result(result)
                    total += 1

                    if result.passed:
                        passed += 1

    except SerialException as error:
        available_ports = ", ".join(port.device for port in list_ports.comports())
        if not available_ports:
            available_ports = "none detected"

        _logger.error("%s\n%s\n%s\n",
            red(f"UART error on {port}: {error}."),
            red(f"Check that the board is connected, update the {prefix}_port in board.ini"),
            red(f"if needed. Available serial ports: {available_ports}"))
        raise SystemExit(1) from None

    finally:
        if echo_server_proc is not None:
            echo_server_proc.terminate()

            try:
                echo_server_proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                echo_server_proc.kill()

    print("\n=============================================")
    print("SUMMARY")
    print("=============================================\n")
    print(green(f"Passed : {passed}"))
    print(red(f"Failed : {total - passed}"))
    print(green(f"Total  : {total}"))

    raise SystemExit(0 if passed == total else 1)


if __name__ == "__main__":
    main()
