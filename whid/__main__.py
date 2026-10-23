"""Entry point: python3 -m whid"""

import sys

from .backup import Backup
from .tracker import Tracker
from .tray import Tray
from .window import Adw, QuickAdd, Window

APP_ID = "io.github.whid.Whid"


def _startup(app: Adw.Application) -> None:
    # One tracker shared by the window and the tray.
    tracker = Tracker()
    app.window = Window(app, tracker)
    app.quick = QuickAdd(app, tracker)
    app.tray = Tray(tracker, APP_ID,
                    on_add=app.quick.show,
                    on_open=app.window.present,
                    on_quit=app.quit)

    # Last: restoring the backup notifies the window and tray.
    app.backup = Backup(app, tracker)


def _activate(app: Adw.Application) -> None:
    # Single instance: a second launch shows the existing window.
    app.window.present()


def main() -> int:
    app = Adw.Application(application_id=APP_ID)
    app.connect("startup", _startup)
    app.connect("activate", _activate)
    return app.run(sys.argv)


if __name__ == "__main__":
    sys.exit(main())
