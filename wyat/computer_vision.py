"""Computer vision utilities used by the broadcast system.

The original monolithic script contained a large `ComputerVisionAnalyzer`
class. Here we provide a lightweight adaptation that exposes the same
interface while keeping dependencies optional.
"""
from __future__ import annotations

import logging
from typing import Dict, List, Optional

import numpy as np

try:  # pragma: no cover - optional dependency
    import cv2
    import torch
    from torchvision import transforms
    from torchvision.models.detection import fasterrcnn_resnet50_fpn
    CV_AVAILABLE = True
except Exception:  # pragma: no cover - warn when CV stack unavailable
    CV_AVAILABLE = False
    logging.getLogger(__name__).warning(
        "Computer vision modules not available. Install opencv-python, torch, torchvision."
    )


class ComputerVisionAnalyzer:
    """Perform object and scene analysis on video frames."""

    def __init__(self) -> None:
        self.device = "cpu"
        if CV_AVAILABLE and torch.cuda.is_available():
            self.device = "cuda"
        self.transforms = transforms.Compose([transforms.ToTensor()]) if CV_AVAILABLE else None
        self.model = None
        if CV_AVAILABLE:
            self._load_model()

    def _load_model(self) -> None:
        if not CV_AVAILABLE:
            return
        self.model = fasterrcnn_resnet50_fpn(pretrained=True)
        self.model.to(self.device)
        self.model.eval()

    def analyze_frame(self, frame: np.ndarray) -> Dict[str, int]:
        """Return a simple summary of detected objects in *frame*."""
        if not (CV_AVAILABLE and self.model and self.transforms):
            return {}
        tensor = self.transforms(frame).to(self.device)
        with torch.no_grad():
            prediction = self.model([tensor])[0]
        labels = prediction.get("labels", [])
        counts: Dict[str, int] = {}
        for label in labels.cpu().numpy():
            counts[str(int(label))] = counts.get(str(int(label)), 0) + 1
        return counts
