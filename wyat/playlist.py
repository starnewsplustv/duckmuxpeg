"""Playlist automation using the :class:`DatabaseManager`."""
from __future__ import annotations

import threading
import time
from dataclasses import dataclass, field
from datetime import datetime, timedelta
from typing import Dict, List, Optional

from .database_manager import DatabaseManager


@dataclass
class Playlist:
    name: str
    channel_id: int
    schedule: Dict[str, object]
    items: List[Dict[str, object]] = field(default_factory=list)
    id: Optional[int] = None


class PlaylistAutomation:
    """Simple scheduler that activates playlists based on time."""

    def __init__(self, db: DatabaseManager) -> None:
        self.db = db
        self.active: Dict[int, Dict[str, object]] = {}
        self._running = False
        self._thread: Optional[threading.Thread] = None

    def start(self) -> None:
        if self._running:
            return
        self._running = True
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._running = False
        if self._thread:
            self._thread.join()

    def _loop(self) -> None:
        while self._running:
            now = datetime.now()
            playlists = self.db.get_playlists()
            for playlist in playlists:
                if self._should_play(playlist, now):
                    self.active[playlist["id"]] = {"playlist": playlist, "index": 0}
            time.sleep(30)

    def _should_play(self, playlist: Dict[str, object], now: datetime) -> bool:
        schedule = playlist["schedule"]
        if schedule.get("type") == "continuous":
            return True
        if schedule.get("type") == "daily":
            start = datetime.strptime(schedule["start_time"], "%H:%M").time()
            end = datetime.strptime(schedule["end_time"], "%H:%M").time()
            current = now.time()
            if start <= end:
                return start <= current <= end
            return current >= start or current <= end
        if schedule.get("type") == "once":
            sched_dt = datetime.strptime(f"{schedule['date']} {schedule['time']}", "%Y-%m-%d %H:%M")
            duration = sum(item.get("duration", 0) for item in playlist["items"])
            return sched_dt <= now <= sched_dt + timedelta(seconds=duration)
        return False
