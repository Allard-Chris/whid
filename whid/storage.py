"""JSON persistence of a work day."""

import json
import shutil
from datetime import date, datetime
from pathlib import Path

from .model import Event, Kind, Task, fmt, parse

VERSION = 2
INDENT = 2
TIMESPEC = "seconds"  # "2026-10-22T10:37:12"
DAYS = ("lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi",
        "dimanche")
TMP_SUFFIX = ".tmp"

# Everything stays in the application folder:
#   whid/.cache/  recovery file + temporary writes
#   whid/export/  default folder of manual exports
ROOT = Path(__file__).resolve().parent.parent
CACHE_DIR = ROOT / ".cache"
EXPORT_DIR = ROOT / "export"


def filename() -> str:
    # "whid-2026-10-22.json"
    return f"whid-{date.today().isoformat()}.json"


def backup_path() -> str:
    CACHE_DIR.mkdir(exist_ok=True)
    return str(CACHE_DIR / filename())


def export_dir() -> str:
    EXPORT_DIR.mkdir(exist_ok=True)
    return str(EXPORT_DIR)


def save(path: str, tasks: list[Task]) -> None:
    # Snapshot: a running task is saved with its current elapsed time.
    today = date.today()
    doc = {
        "version": VERSION,
        "date": today.isoformat(),
        "day": DAYS[today.weekday()],
        "week": today.isocalendar().week,
        "tasks": [_entry(t) for t in tasks],
        "total": fmt(sum(t.elapsed() for t in tasks)),
    }

    # Write in the cache then move: a kill mid-write never corrupts the
    # file. Same filesystem -> atomic rename; otherwise copy.
    CACHE_DIR.mkdir(exist_ok=True)
    tmp = CACHE_DIR / (Path(path).name + TMP_SUFFIX)
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(doc, f, ensure_ascii=False, indent=INDENT)

    shutil.move(tmp, path)


def load(path: str) -> list[Task]:
    try:
        with open(path, encoding="utf-8") as f:
            doc = json.load(f)
        return [_task(e) for e in doc["tasks"]]
    except (OSError, ValueError, KeyError, TypeError) as e:
        raise ValueError(f"invalid file: {path}") from e


def _stamp(moment: datetime | None) -> str | None:
    if moment is None:
        return None

    return moment.isoformat(timespec=TIMESPEC)


def _entry(task: Task) -> dict:
    return {
        "title": task.title,
        "description": task.description,
        "created": _stamp(task.created),
        "duration": fmt(task.elapsed()),
        "events": [{"type": e.kind.value, "at": _stamp(e.at)}
                   for e in task.events],
    }


def _task(entry: dict) -> Task:
    # Version 1 files have no "created" nor "events".
    created = entry.get("created")
    events = [Event(Kind(e["type"]), datetime.fromisoformat(e["at"]))
              for e in entry.get("events", [])]

    return Task(
        title=str(entry["title"]),
        description=str(entry.get("description", "")),
        seconds=parse(entry["duration"]),
        created=datetime.fromisoformat(created) if created else None,
        events=events,
    )
