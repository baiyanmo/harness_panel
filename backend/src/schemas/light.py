"""灯光控制 Schema"""
from pydantic import BaseModel


class ControlRequest(BaseModel):
    """通用控制请求"""
    state: bool
