"""Database management utilities for WYAT broadcast system.

Provides persistent storage using SQLite for settings, channels, playlists,
 and analytics data. This module is adapted from the monolithic script and
exposed as a reusable component.
"""
from __future__ import annotations

import json
import sqlite3
from pathlib import Path
from typing import Any, Dict, List, Optional

CONFIG_DIR = Path("config")
DB_PATH = CONFIG_DIR / "wyat_broadcast.db"


class DatabaseManager:
    """Manages persistent storage for settings and configurations."""

    def __init__(self) -> None:
        CONFIG_DIR.mkdir(exist_ok=True)
        self._init_database()

    def _init_database(self) -> None:
        """Initialise the SQLite database with required tables."""
        conn = sqlite3.connect(DB_PATH)
        cur = conn.cursor()
        cur.execute(
            """
            CREATE TABLE IF NOT EXISTS settings (
                key TEXT PRIMARY KEY,
                value TEXT,
                updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """
        )
        cur.execute(
            """
            CREATE TABLE IF NOT EXISTS channels (
                id INTEGER PRIMARY KEY,
                name TEXT,
                config TEXT,
                enabled BOOLEAN DEFAULT 1,
                updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """
        )
        conn.commit()
        conn.close()

    def save_setting(self, key: str, value: Any) -> None:
        """Persist a setting value."""
        conn = sqlite3.connect(DB_PATH)
        cur = conn.cursor()
        cur.execute(
            "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)",
            (key, json.dumps(value)),
        )
        conn.commit()
        conn.close()

    def get_setting(self, key: str, default: Any | None = None) -> Any | None:
        """Retrieve a previously stored setting value."""
        conn = sqlite3.connect(DB_PATH)
        cur = conn.cursor()
        cur.execute("SELECT value FROM settings WHERE key = ?", (key,))
        row = cur.fetchone()
        conn.close()
        if row:
            return json.loads(row[0])
        return default

    def save_channel_config(self, channel_id: int, config: Dict[str, Any]) -> None:
        """Persist configuration for a channel."""
        conn = sqlite3.connect(DB_PATH)
        cur = conn.cursor()
        cur.execute(
            """
            INSERT OR REPLACE INTO channels (id, name, config, enabled)
            VALUES (?, ?, ?, ?)
            """,
            (
                channel_id,
                config.get("name", f"Channel {channel_id}"),
                json.dumps(config),
                config.get("enabled", True),
            ),
        )
        conn.commit()
        conn.close()

    def get_channel_config(self, channel_id: int) -> Optional[Dict[str, Any]]:
        """Return stored configuration for a channel."""
        conn = sqlite3.connect(DB_PATH)
        cur = conn.cursor()
        cur.execute("SELECT config FROM channels WHERE id = ?", (channel_id,))
        row = cur.fetchone()
        conn.close()
        if row:
            return json.loads(row[0])
        return None
