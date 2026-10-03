"""Shared helper for running external commands."""

import logging
import subprocess
from collections.abc import Sequence
from typing import Final

from colors import red


class SubprocessRunner:
    """Run external commands and report command failures."""

    def __init__(self, logger: logging.Logger) -> None:
        self._logger: Final[logging.Logger] = logger

    def start(self,
              command: Sequence[str],
              stdout: int | None = None,
              stderr: int | None = None) -> subprocess.Popen[bytes]:
        """Start a long-running command and return its process handle."""
        try:
            return subprocess.Popen(command, stdout=stdout, stderr=stderr)
        except OSError as error:
            self._logger.error("%s", red(f"Unable to start command: {error}"))
            raise RuntimeError("Unable to start subprocess.") from error

    def run(self,
            command: Sequence[str],
            failure_message: str) -> subprocess.CompletedProcess[str]:
        """Run a command, logging captured output if it exits unsuccessfully."""
        try:
            return subprocess.run(command,
                                  check=True,
                                  capture_output=True,
                                  text=True,
                                  encoding="utf-8",
                                  errors="replace")
        except subprocess.CalledProcessError as error:
            self._logger.error("%s", red(f"Return code: {error.returncode}"))
            self._logger.error("%s", red(f"STDERR: {error.stderr}"))
            self._logger.error("%s", red(f"STDOUT: {error.stdout}"))
            raise RuntimeError(red(failure_message)) from error
