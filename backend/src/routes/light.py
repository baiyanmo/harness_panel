"""灯光控制路由"""
from fastapi import APIRouter, HTTPException
from ..schemas.light import ControlRequest
from ..tools.bafa_api import send_msg

router = APIRouter()

LIGHT_TOPIC = "LIGHT002"


def _light_msg(state: bool) -> str:
    return "on" if state else "off"


@router.post("/light/control")
async def control_light(req: ControlRequest):
    msg = _light_msg(req.state)
    if not await send_msg(LIGHT_TOPIC, msg):
        raise HTTPException(status_code=500, detail="Failed to control light via BaFa Cloud")
    return {"status": "success", "state": req.state}
