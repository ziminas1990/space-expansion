#!/usr/bin/env python3
"""Create and close task files in the current directory.

Usage, from a directory that contains tasks.json:
  python scripts/task.py create
  python scripts/task.py close TASK

The script uses the working directory it was started in, not the
directory that contains this file. tasks.json in that working directory
sets the task suffix (SES, TSSDK) and the subdirectory for closed tasks
(closed, when omitted).

close moves the file into that subdirectory and rewrites markdown links
that point at it. Links are updated in every other document under the
working directory, including tasks already closed and files in
subdirectories such as epics/. The closed task's own relative links are
adjusted so they still point at the same targets.
"""

import json
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path

CONFIG_NAME = "tasks.json"
DEFAULT_CLOSED = "closed"
SUFFIX_NAME = re.compile(r"^[A-Z][A-Z0-9]*$")
ANY_TASK = re.compile(r"^([A-Z][A-Z0-9]*)-(\d+)\.md$")
MARKDOWN_LINK = re.compile(r"\]\(([^)\s]+)([^)]*)\)")

TEMPLATE = """\
# {suffix}-{number}:

## Background

## Require

## Acceptance

-
"""


@dataclass(frozen=True)
class Tracker:
    root: Path
    suffix: str
    closed_name: str

    @property
    def closed_dir(self) -> Path:
        return self.root / self.closed_name

    def task_pattern(self) -> re.Pattern[str]:
        return re.compile(rf"^{re.escape(self.suffix)}-(\d+)\.md$")


def fail(message: str, code: int = 1) -> None:
    print(message, file=sys.stderr)
    sys.exit(code)


def usage() -> None:
    fail(
        "usage: python scripts/task.py create\n"
        "       python scripts/task.py close TASK\n"
        "\n"
        "Run from a directory that contains tasks.json.",
        2,
    )


def closed_dir_name(value: object) -> str:
    if not isinstance(value, str) or value in ("", ".", ".."):
        fail(f"{CONFIG_NAME}: closed must be a directory name")
    if "/" in value or "\\" in value:
        fail(f"{CONFIG_NAME}: closed must be a single directory name")
    return value


def load_tracker(root: Path) -> Tracker:
    path = root / CONFIG_NAME
    if not path.is_file():
        fail(f"no {CONFIG_NAME} in {root}")
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        fail(f"invalid {CONFIG_NAME}: {error}")
    if not isinstance(data, dict):
        fail(f"{CONFIG_NAME} must be an object")
    unknown = set(data) - {"suffix", "closed"}
    if unknown:
        names = ", ".join(sorted(unknown))
        fail(f"{CONFIG_NAME}: unknown settings: {names}")
    suffix = data.get("suffix")
    if not isinstance(suffix, str) or not SUFFIX_NAME.fullmatch(suffix):
        fail(f"{CONFIG_NAME}: suffix must look like SES or TSSDK")
    if "closed" in data:
        closed_name = closed_dir_name(data["closed"])
    else:
        closed_name = DEFAULT_CLOSED
    return Tracker(root, suffix, closed_name)


def task_folders(tracker: Tracker):
    yield tracker.root
    if tracker.closed_dir.is_dir():
        yield tracker.closed_dir


def iter_task_files(tracker: Tracker):
    pattern = tracker.task_pattern()
    for folder in task_folders(tracker):
        for path in folder.glob("*.md"):
            if pattern.match(path.name):
                yield path


def check_suffix(tracker: Tracker) -> None:
    for folder in task_folders(tracker):
        for path in folder.glob("*.md"):
            match = ANY_TASK.match(path.name)
            if match and match.group(1) != tracker.suffix:
                fail(
                    f"{path.name} does not use suffix {tracker.suffix} "
                    f"from {CONFIG_NAME}"
                )


def highest_number(tracker: Tracker) -> int:
    pattern = tracker.task_pattern()
    highest = 0
    for path in iter_task_files(tracker):
        match = pattern.match(path.name)
        if match:
            highest = max(highest, int(match.group(1)))
    return highest


def create_task(tracker: Tracker) -> str:
    check_suffix(tracker)
    number = highest_number(tracker)
    while True:
        number += 1
        label = f"{number:03d}"
        name = f"{tracker.suffix}-{label}.md"
        path = tracker.root / name
        try:
            with path.open("x", encoding="utf-8") as handle:
                handle.write(TEMPLATE.format(suffix=tracker.suffix, number=label))
        except FileExistsError:
            continue
        return name


def task_filename(tracker: Tracker, name: str) -> str:
    if "/" in name or "\\" in name:
        fail("pass a task name, not a path")
    filename = name if name.endswith(".md") else f"{name}.md"
    if not tracker.task_pattern().match(filename):
        fail(f"not a {tracker.suffix} task: {name}")
    return filename


def find_open_task(tracker: Tracker, name: str) -> Path:
    filename = task_filename(tracker, name)
    path = tracker.root / filename
    if path.is_file():
        return path
    if (tracker.closed_dir / filename).is_file():
        fail(f"{filename} is already in {tracker.closed_name}/")
    fail(f"no such task: {filename}")


def split_url(url: str) -> tuple[str, str]:
    if url.startswith("#"):
        return "", url
    hash_at = url.find("#")
    if hash_at == -1:
        return url, ""
    return url[:hash_at], url[hash_at:]


def is_local(path: str) -> bool:
    if path == "" or path.startswith(("#", "/")):
        return False
    return "://" not in path


def relative_link(target: Path, from_dir: Path) -> str:
    relative = os.path.relpath(target, from_dir).replace(os.sep, "/")
    if not relative.startswith("."):
        relative = "./" + relative
    return relative


def rewrite_links(
    text: str,
    link_base: Path,
    output_dir: Path,
    old_path: Path,
    new_path: Path,
    *,
    only_moved_target: bool,
) -> str:
    old_resolved = old_path.resolve()
    new_resolved = new_path.resolve()

    def replace(match: re.Match[str]) -> str:
        url = match.group(1)
        path, anchor = split_url(url)
        if not is_local(path):
            return match.group(0)
        target = (link_base / path).resolve()
        if target == old_resolved:
            target = new_resolved
        elif only_moved_target:
            return match.group(0)
        rewritten = relative_link(target, output_dir) + anchor
        if rewritten == url:
            return match.group(0)
        return f"]({rewritten}{match.group(2)})"

    return MARKDOWN_LINK.sub(replace, text)


def close_task(tracker: Tracker, name: str) -> str:
    check_suffix(tracker)
    source = find_open_task(tracker, name)
    destination_dir = tracker.closed_dir
    destination = destination_dir / source.name
    if destination.exists():
        fail(f"{tracker.closed_name}/{source.name} already exists")

    updates: list[tuple[Path, str]] = []
    for path in tracker.root.rglob("*.md"):
        if path.resolve() == source.resolve():
            continue
        original = path.read_text(encoding="utf-8")
        rewritten = rewrite_links(
            original,
            path.parent,
            path.parent,
            source,
            destination,
            only_moved_target=True,
        )
        if rewritten != original:
            updates.append((path, rewritten))

    moved_text = rewrite_links(
        source.read_text(encoding="utf-8"),
        source.parent,
        destination_dir,
        source,
        destination,
        only_moved_target=False,
    )

    destination_dir.mkdir(exist_ok=True)
    destination.write_text(moved_text, encoding="utf-8")
    for path, text in updates:
        path.write_text(text, encoding="utf-8")
    source.unlink()
    return f"{tracker.closed_name}/{source.name}"


def main(argv: list[str]) -> None:
    if len(argv) < 2:
        usage()
    command = argv[1]
    tracker = load_tracker(Path.cwd().resolve())
    if command == "create":
        if len(argv) != 2:
            usage()
        print(create_task(tracker))
        return
    if command == "close":
        if len(argv) != 3:
            usage()
        print(close_task(tracker, argv[2]))
        return
    usage()


if __name__ == "__main__":
    main(sys.argv)
