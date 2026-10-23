#!/bin/sh
# Add a GNOME launcher pointing to this folder.
set -e

APP_ID="io.github.whid.Whid"
DIR="$(cd "$(dirname "$0")" && pwd)"
DEST="$HOME/.local/share/applications/$APP_ID.desktop"
ICONS="$HOME/.local/share/icons/hicolor"
ICON_SIZE="48x48"

# Icon named after APP_ID: GNOME matches it to the window and launcher.
mkdir -p "$ICONS/$ICON_SIZE/apps"
cp "$DIR/data/$APP_ID.png" "$ICONS/$ICON_SIZE/apps/"
gtk-update-icon-cache -q -t "$ICONS" 2>/dev/null || true

mkdir -p "$(dirname "$DEST")"
cat > "$DEST" <<DESKTOP
[Desktop Entry]
Type=Application
Name=Whid
Comment=Suivi du temps par tâche
Exec=python3 -m whid
Path=$DIR
Icon=$APP_ID
Terminal=false
Categories=Utility;
DESKTOP

echo "Installé : $DEST"
