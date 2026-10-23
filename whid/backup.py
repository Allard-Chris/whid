"""Automatic backup in .cache/: never lose a day of work.

  start                         -> restore .cache/whid-<today>.json
  any change, every minute      -> save (survives SIGKILL, power loss)
  quit, SIGTERM, SIGINT, SIGHUP -> save + export/whid-<today>.json
"""

import signal
import sys
from pathlib import Path

from gi.repository import Gio, GLib

from .tracker import Tracker

AUTOSAVE_SECONDS = 60
QUIT_SIGNALS = (signal.SIGTERM, signal.SIGINT, signal.SIGHUP)


class Backup:
    def __init__(self, app: Gio.Application, tracker: Tracker) -> None:
        self.tracker = tracker
        self._restore()

        tracker.listen(self._write)
        GLib.timeout_add_seconds(AUTOSAVE_SECONDS, self._tick)
        app.connect("shutdown", self._shutdown)

        # Signals end the app cleanly, so "shutdown" saves.
        for sig in QUIT_SIGNALS:
            GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, sig, self._quit, app)

    def save(self) -> None:
        # Nothing to save: keep any existing file intact.
        if not self.tracker.tasks:
            return

        self._write()

    def _write(self) -> None:
        try:
            self.tracker.export(self._path())
        except OSError as e:
            print(f"whid: backup failed: {e}", file=sys.stderr)

    def _path(self) -> str:
        # Computed on each save: a new day gets a new file.
        return self.tracker.backup_path()

    def _restore(self) -> None:
        path = self._path()
        if not Path(path).exists():
            return

        try:
            self.tracker.load(path)
        except ValueError as e:
            print(f"whid: backup not restored: {e}", file=sys.stderr)

    def _tick(self) -> bool:
        self.save()
        return GLib.SOURCE_CONTINUE

    def _shutdown(self, _app: Gio.Application) -> None:
        # Leaving the app exports the day as is, like a manual export.
        self.save()
        if not self.tracker.tasks:
            return

        path = str(Path(self.tracker.export_dir()) / self.tracker.filename())
        try:
            self.tracker.export(path)
        except OSError as e:
            print(f"whid: export failed: {e}", file=sys.stderr)

    def _quit(self, app: Gio.Application) -> bool:
        app.quit()
        return GLib.SOURCE_REMOVE
