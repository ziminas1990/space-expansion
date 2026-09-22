#!/usr/bin/env python3
"""Allocate the next SES task file and print its name."""

import re
import sys
from pathlib import Path

TASKS_DIR = Path(__file__).resolve().parent
TASK_NAME = re.compile(r"^SES-(\d+)\.md$")

TEMPLATE = """\
# SES-{number}:

## Background

## Require

## Acceptance

-
"""


def highest_number() -> int:
    highest = 0
    for path in TASKS_DIR.rglob("*.md"):
        match = TASK_NAME.match(path.name)
        if match:
            highest = max(highest, int(match.group(1)))
    return highest


def create_task() -> str:
    number = highest_number()
    while True:
        number += 1
        label = f"{number:03d}"
        name = f"SES-{label}.md"
        path = TASKS_DIR / name
        try:
            with path.open("x", encoding="utf-8") as handle:
                handle.write(TEMPLATE.format(number=label))
        except FileExistsError:
            continue
        return name


def main() -> None:
    if len(sys.argv) != 1:
        print("usage: create_task.py", file=sys.stderr)
        sys.exit(2)
    print(create_task())


if __name__ == "__main__":
    main()
