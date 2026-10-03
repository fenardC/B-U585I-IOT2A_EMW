
"""."""

import time

from serial import Serial

from flash_tool import FlashTool
from test_types import (
    TestCase,
    TestResult,
)


def run_test(uart: Serial,
             test: TestCase,
             flash_tool: FlashTool,
             timeout_s: float) -> TestResult:
    """Run a single test."""
    if test.read_symbol is not None or test.read_address is not None:
        address: int

        if test.read_symbol is not None:
            address = flash_tool.get_symbol_address(test.read_symbol)

        else:
            assert test.read_address is not None
            address = test.read_address

        value: int

        if test.read_type == "u8":
            value = flash_tool.read_u8(address)

        elif test.read_type == "u16":
            value = flash_tool.read_u16(address)

        else:
            value = flash_tool.read_u32(address)

        passed: bool = True

        if test.expect_value is not None:
            passed = value == test.expect_value

        if passed and test.expect_min is not None:
            passed = value >= test.expect_min

        if passed and test.expect_max is not None:
            passed = value <= test.expect_max

        return TestResult(name=test.name,
                          passed=passed,
                          expected=str(test.expect_value),
                          actual=str(value),
                          missing=str(test.expect_value))

    response: str = ""

    uart.reset_input_buffer()

    if test.send:
        uart.write(f"{test.send}\r\n".encode())
        uart.flush()

    start_time: float = time.monotonic()

    while time.monotonic() - start_time < timeout_s:
        line: str = uart.readline().decode(errors="replace").strip()

        if line:
            response += (line + "\n")

        if test.end_marker and test.end_marker in response:
            break

    passed: bool = True
    expected: str = ", ".join(test.expects)
    missing: list[str] = []

    for text in test.expects:
        if text not in response:
            passed = False
            missing.append(text)

    return TestResult(name=test.name,
                      passed=passed,
                      expected=expected,
                      actual=response,
                      missing=", ".join(missing))
