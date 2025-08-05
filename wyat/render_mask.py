"""Motion region analysis and mask generation utilities."""
from __future__ import annotations

import time
from collections import deque
from typing import Dict, List, Tuple

import cv2
import numpy as np


class RenderMaskGenerator:
    """Analyse video frames to derive static and dynamic regions.

    This is a modular extraction of the original monolithic implementation
    providing helpers for motion detection and mask creation used by encoders.
    """

    def __init__(self, history_size: int = 150) -> None:
        self.prev_frame: np.ndarray | None = None
        self.motion_history: deque = deque(maxlen=history_size)

    def analyse(self, frame: np.ndarray) -> Dict[str, List[Dict[str, int]]]:
        """Return dictionaries describing static and dynamic regions."""
        if self.prev_frame is None:
            self.prev_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            return {"static": [], "dynamic": [], "motion_percentage": 0.0}

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        diff = cv2.absdiff(self.prev_frame, gray)
        _, thresh = cv2.threshold(diff, 25, 255, cv2.THRESH_BINARY)
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        h, w = frame.shape[:2]
        static_mask = np.ones((h, w), dtype=np.uint8) * 255
        dynamic_regions: List[Dict[str, int]] = []
        for c in contours:
            area = cv2.contourArea(c)
            if area > 100:
                x, y, w_, h_ = cv2.boundingRect(c)
                dynamic_regions.append({"bbox": [x, y, w_, h_], "area": int(area)})
                cv2.rectangle(static_mask, (x, y), (x + w_, y + h_), 0, -1)

        static_contours, _ = cv2.findContours(static_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        static_regions: List[Dict[str, int]] = []
        for c in static_contours:
            area = cv2.contourArea(c)
            if area > 500:
                x, y, w_, h_ = cv2.boundingRect(c)
                static_regions.append({"bbox": [x, y, w_, h_], "area": int(area)})

        self.prev_frame = gray
        self.motion_history.append({
            "timestamp": time.time(),
            "dynamic_area": sum(r["area"] for r in dynamic_regions),
            "static_area": sum(r["area"] for r in static_regions),
        })

        return {
            "static": static_regions,
            "dynamic": dynamic_regions,
            "motion_percentage": self.motion_percentage(),
        }

    def motion_percentage(self) -> float:
        """Average percentage of motion based on history."""
        if not self.motion_history:
            return 0.0
        total = sum(h["dynamic_area"] + h["static_area"] for h in self.motion_history)
        dynamic = sum(h["dynamic_area"] for h in self.motion_history)
        if total == 0:
            return 0.0
        return (dynamic / total) * 100

    def generate_mask(self, frame_shape: Tuple[int, int], regions: Dict[str, List[Dict[str, int]]]) -> np.ndarray:
        """Create a quality mask highlighting dynamic regions."""
        h, w = frame_shape[:2]
        mask = np.ones((h, w), dtype=np.float32) * 0.5
        for region in regions.get("dynamic", []):
            x, y, w_, h_ = region["bbox"]
            mask[y:y + h_, x:x + w_] = 1.0
        for region in regions.get("static", []):
            x, y, w_, h_ = region["bbox"]
            mask[y:y + h_, x:x + w_] = 0.3
        return mask
