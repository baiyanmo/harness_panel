"""聊天相关 Schema"""
from pydantic import BaseModel
from typing import List, Optional, Literal


class ChatMessage(BaseModel):
    """单条消息"""
    role: Literal["user", "assistant", "system"]
    content: str


class ChatRequest(BaseModel):
    """POST /api/chat 请求体"""
    messages: List[ChatMessage]
    stream: bool = True
    max_tokens: int = 1024
    temperature: float = 0.7


class ChatOut(BaseModel):
    """非流式响应"""
    role: str = "assistant"
    content: str
    model: Optional[str] = None
    error: Optional[str] = None
    emotion: Optional[str] = None   # Live2D 表情
    action: Optional[str] = None    # Live2D 动作
    light: Optional[bool] = None    # 灯光开关状态
