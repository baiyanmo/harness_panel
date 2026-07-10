"""灯光控制 Schema"""
from pydantic import BaseModel


class ControlRequest(BaseModel):
    """灯光颜色控制请求：red / green / blue / off"""
    color: str
