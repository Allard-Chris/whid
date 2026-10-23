"""Service layer: past days, read from report files."""

from datetime import date

from . import storage
from .model import Task


class Reports:
    def __init__(self) -> None:
        self.files: dict[date, str] = {}

    def days(self) -> set[date]:
        # Rescan: new exports and today's autosave show up.
        self.files = storage.reports()
        return set(self.files)

    def tasks(self, day: date) -> list[Task]:
        # Raises ValueError if the file vanished or broke since the scan.
        return storage.read(self.files[day])[1]
