"""Color methods for strings."""

def cyan(text: str) -> str:
    """Format text in cyan."""
    return f"\033[96m{text}\033[0m"

def green(text: str) -> str:
    """Format text in green."""
    return f"\033[92m{text}\033[0m"


def red(text: str) -> str:
    """Format text in red."""
    return f"\033[91m{text}\033[0m"


def yellow(text: str) -> str:
    """Format text in yellow."""
    return f"\033[93m{text}\033[0m"
