"""Utility for flash image of the STM32 board."""

import logging
import re
import tempfile
from pathlib import Path
from typing import Final

from colors import cyan, red
from subprocess_runner import SubprocessRunner


class FlashTool:
    """STM32 flash helper."""
    def __init__(self,
                 programmer: Path,
                 serial_number: str,
                 map_file: Path,
                 logger: logging.Logger) -> None:
        """Initialize flash tool."""
        self._programmer: Final[Path] = programmer
        self._serial_number: Final[str] = serial_number
        self._map_file: Final[Path] = map_file
        self.logger: logging.Logger = logger
        self._subprocess_runner: Final[SubprocessRunner] = SubprocessRunner(logger)

    def flash(self,
              firmware: Path | str,
              address: str | None = None) -> None:
        """Flash firmware."""
        firmware_path = firmware.as_posix() if isinstance(firmware, Path) else firmware
        is_binary: bool = Path(firmware_path).suffix.lower() == ".bin"

        command: list[str] = [self._programmer.as_posix()]
        command.extend(["-c", f"port=SWD sn={self._serial_number}"])

        if is_binary and address is not None:
            command.extend(["-d", firmware_path, address])
        else:
            command.extend(["-d", firmware_path])

        command.extend(["-v", "-rst"])

        target = f"{firmware_path} @ {address}" \
        if is_binary and address is not None else firmware_path

        self.logger.info("\n%s\n",
                         cyan(f"Flashing {target}"))

        self.logger.debug("\n%s\n",
                          cyan(f"Command: {' '.join(command)}"))

        self._subprocess_runner.run(command, "Unable to flash firmware.")

    def get_symbol_address(self,
                           symbol: str) -> int:
        """Get symbol address from map file."""
        pattern: re.Pattern[str] = re.compile(rf"^(0x[0-9a-fA-F]+)\s+{re.escape(symbol)}$")

        with self._map_file.open(encoding="utf-8",
                                 errors="ignore") as file:
            for line in file:
                match = pattern.search(line.strip())

                if match is not None:
                    return int(match.group(1), 16)

        raise ValueError(red(f"Symbol not found: {symbol}"))


    def read_memory(self,
                    address: int,
                    length: int) -> bytes:
        """Read target memory using STM32_Programmer_CLI."""
        with tempfile.NamedTemporaryFile(suffix=".bin",
                                         delete=False) as file:
            output_file = Path(file.name)

        command: list[str] = [self._programmer.as_posix(),
            "-c",
            f"port=SWD sn={self._serial_number}",
            "-r8",
            hex(address),
            str(length),
            str(output_file)]
        try:
            self._subprocess_runner.run(command, "Unable to read target memory.")
            data = output_file.read_bytes()
        finally:
            output_file.unlink(missing_ok=True)

        if len(data) < length:
            raise RuntimeError(red("Unable to read target memory."))

        return data[:length]

    def read_u8(self,
                address: int) -> int:
        """."""
        return int.from_bytes(self.read_memory(address, 1),
                              byteorder="little" )

    def read_u16(self,
                 address: int) -> int:
        """."""
        return int.from_bytes(self.read_memory(address, 2),
                              byteorder="little")

    def read_u32(self,
                 address: int) -> int:
        """."""
        return int.from_bytes(self.read_memory(address, 4),
                              byteorder="little")

    def read_variable_u8(self,
                         symbol: str) -> int:
        """."""
        return self.read_u8(self.get_symbol_address(symbol))

    def read_variable_u16(self,
                          symbol: str) -> int:
        """."""
        return self.read_u16(self.get_symbol_address(symbol))

    def read_variable_u32(self,
                          symbol: str) -> int:
        """."""
        return self.read_u32(self.get_symbol_address(symbol))
