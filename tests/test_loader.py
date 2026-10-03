"""."""

import configparser
from pathlib import Path

from test_types import TestCase


def load_test_case(config: configparser.ConfigParser,
                   section_name: str) -> TestCase:
    """Load a test case from test.ini."""
    section: configparser.SectionProxy = config[section_name]

    expects: list[str] = []

    key: str

    for key in sorted(
        (candidate for candidate in section.keys() if candidate.startswith("expect_")),
        key=lambda candidate: int(candidate.split("_", 1)[1]),
    ):
        expects.append(section[key])

    firmware: list[Path] = []

    if "firmware" in section:
        firmware = \
        [item.strip() for item in section["firmware"].split(",") if item.strip()]

    read_address: int | None = None

    if "read_address" in section:
        read_address = int(section["read_address"], 0)

    expect_value: int | None = None

    if "expect_value" in section:
        expect_value = int(section["expect_value"], 0)

    expect_min: int | None = None

    if "expect_min" in section:
        expect_min = int(section["expect_min"], 0)

    expect_max: int | None = None

    if "expect_max" in section:
        expect_max = int(section["expect_max"], 0)

    return TestCase(name=section.get("name", section_name),
                    send=section.get("send"),
                    end_marker=section.get("end_marker"),
                    expects=expects,
                    firmware_concerned=firmware,
                    read_symbol=section.get("read_symbol"),
                    read_address=read_address,
                    read_type=section.get("read_type", "u32"),
                    expect_value=expect_value,
                    expect_min=expect_min,
                    expect_max=expect_max)
