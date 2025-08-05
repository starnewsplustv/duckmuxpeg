"""Entry point demonstrating modular components."""
from __future__ import annotations

import numpy as np
from . import DatabaseManager, SceneDetector


def demo() -> None:
    db = DatabaseManager()
    db.save_setting("demo", {"active": True})
    detector = SceneDetector()
    # Generate dummy frame
    frame = np.zeros((480, 640, 3), dtype=np.uint8)
    change, diff = detector.detect(frame)
    print("Scene changed?", change, "diff:", diff)


if __name__ == "__main__":
    demo()
