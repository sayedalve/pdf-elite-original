from __future__ import annotations

from pdf_elite.models import ApiModel


class HealthResponse(ApiModel):
    status: str
    smart_model: str
    fast_model: str
