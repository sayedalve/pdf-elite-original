from __future__ import annotations

from typing import Annotated

from fastapi import APIRouter, Depends

from pdf_elite.agents import ExecutionPlanningAgent
from pdf_elite.api.dependencies import get_execution_planning_agent
from pdf_elite.contracts import AgentExecutionRequest, NextExecutionAction

router = APIRouter(prefix="/api/v1/agents", tags=["agents"])


@router.post("/next-action", response_model=NextExecutionAction)
async def next_action(
    request: AgentExecutionRequest,
    agent: Annotated[ExecutionPlanningAgent, Depends(get_execution_planning_agent)],
) -> NextExecutionAction:
    return await agent.next_action(request)
