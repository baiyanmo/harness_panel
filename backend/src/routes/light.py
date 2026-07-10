"""灯光控制路由"""
from fastapi import APIRouter, HTTPException
from ..schemas.light import ControlRequest
from ..tools.bafa_api import send_msg, get_msg

router = APIRouter()

LIGHT_TOPIC = "LIGHT002"


@router.get("/light/status")
async def get_light_status():
    """查询巴法云设备上报的灯光状态"""
    msgs = await get_msg(LIGHT_TOPIC)
    if not msgs:
        return {"status": "success", "color": "unknown", "msg": "无设备数据"}
    # 取最新一条消息
    latest = msgs[-1]
    color = latest.get("msg", "unknown")
    return {"status": "success", "color": color, "msg": str(latest)}


@router.post("/light/control")
async def control_light(req: ControlRequest):
    """发送灯光颜色到巴法云：red / green / blue / off"""
    color = req.color.lower()
    if color not in ("red", "green", "blue", "off"):
        raise HTTPException(status_code=400, detail="color must be red, green, blue, or off")
    if not await send_msg(LIGHT_TOPIC, color):
        raise HTTPException(status_code=500, detail="Failed to control light via BaFa Cloud")
    return {"status": "success", "color": color}
