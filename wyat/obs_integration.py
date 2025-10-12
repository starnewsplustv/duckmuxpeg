"""OBS Studio WebSocket integration helpers."""
from __future__ import annotations

from typing import List, Optional

try:
    import obsws_python as obs  # type: ignore
except Exception:  # pragma: no cover - optional dependency
    obs = None


class OBSIntegration:
    """Small wrapper around obsws-python for basic control."""

    def __init__(self) -> None:
        self.client: Optional[obs.ReqClient] = None if obs else None

    def connect(self, host: str = "localhost", port: int = 4455, password: str = "") -> bool:
        if not obs:
            return False
        try:
            self.client = obs.ReqClient(host=host, port=port, password=password)
            return True
        except Exception:
            self.client = None
            return False

    def disconnect(self) -> None:
        if self.client:
            self.client.disconnect()
            self.client = None

    def scenes(self) -> List[str]:
        if not self.client:
            return []
        try:
            response = self.client.get_scene_list()
            return [s["sceneName"] for s in response.scenes]
        except Exception:
            return []

    def switch(self, scene: str) -> bool:
        if not self.client:
            return False
        try:
            self.client.set_current_program_scene(scene)
            return True
        except Exception:
            return False
