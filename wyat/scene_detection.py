"""Scene change detection utilities."""
from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import time
from typing import Deque, Tuple, Dict

import numpy as np
import cv2


@dataclass
class SceneChange:
    timestamp: float
    diff: float
    is_change: bool


class SceneDetector:
    """Detects scene changes using histogram comparison."""

    def __init__(self, threshold: float = 30.0) -> None:
        self.threshold = threshold
        self.prev_hist: np.ndarray | None = None
        self.history: Deque[SceneChange] = deque(maxlen=300)

    def detect(self, frame: np.ndarray) -> Tuple[bool, float]:
        """Return (is_change, diff) for *frame*."""
        hist = cv2.calcHist([frame], [0, 1, 2], None, [32, 32, 32], [0, 256] * 3)
        hist = cv2.normalize(hist, hist).flatten()
        if self.prev_hist is None:
            self.prev_hist = hist
            return False, 0.0
        diff = cv2.compareHist(self.prev_hist, hist, cv2.HISTCMP_CHISQR)
        change = diff > self.threshold
        self.history.append(SceneChange(time.time(), diff, change))
        self.prev_hist = hist
        return change, diff

    def stats(self) -> Dict[str, float]:
        """Return basic statistics about recent scene changes."""
        if not self.history:
            return {"change_rate": 0.0, "avg_diff": 0.0}
        changes = sum(1 for s in self.history if s.is_change)
        avg_diff = float(np.mean([s.diff for s in self.history]))
        return {"change_rate": changes / len(self.history), "avg_diff": avg_diff}
