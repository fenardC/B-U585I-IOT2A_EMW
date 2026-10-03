"""."""

from dataclasses import dataclass


@dataclass(slots=True)
class TestCase:
    """Test definition."""

    name: str
    send: str | None
    end_marker: str | None
    expects: list[str]
    firmware_concerned: list[str]
    read_symbol: str | None
    read_address: int | None
    read_type: str | None
    expect_value: int | None
    expect_min: int | None
    expect_max: int | None


@dataclass(slots=True)
class TestResult:
    """Test execution result."""

    name: str
    passed: bool
    expected: str
    actual: str
    missing: str
