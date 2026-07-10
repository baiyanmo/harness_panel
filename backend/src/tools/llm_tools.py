"""豆包 Function Calling 工具定义 — 供大模型调用的外部能力"""
import json
import logging
from .bafa_api import send_msg
from .actions import Live2DActions

logger = logging.getLogger(__name__)

LIGHT_TOPIC = "LIGHT002"

# ────────────────── 工具 Schema（OpenAI function calling 格式） ──────────────────

LIGHT_TOOL = {
    "type": "function",
    "function": {
        "name": "control_light",
        "description": "控制房间的氛围灯。可以开灯、关灯，或切换灯的颜色（红/绿/蓝）。",
        "parameters": {
            "type": "object",
            "properties": {
                "action": {
                    "type": "string",
                    "enum": ["on", "off"],
                    "description": "开灯或关灯",
                },
                "color": {
                    "type": "string",
                    "enum": ["red", "green", "blue"],
                    "description": "灯的颜色，开灯时指定，默认红色",
                },
            },
            "required": ["action"],
        },
    },
}

LLM_TOOLS = [LIGHT_TOOL]


# ────────────────── 共享执行逻辑 ──────────────────

async def control_light(action: str, color: str = "red") -> tuple[str, str | None, str | None, str | None]:
    """开关灯 / 切换颜色 — 正则和豆包 tool calling 共用。

    Args:
        action: "on" 或 "off"
        color:   "red" / "green" / "blue"（开灯时生效）

    Returns:
        (result_text, emotion, action_name, light_state)
    """
    if action == "off":
        await send_msg(LIGHT_TOPIC, "off")
        emotion, action_name = Live2DActions.doze()
        return ("灯已关闭", emotion, action_name, "off")

    color = color if color in ("red", "green", "blue") else "red"
    await send_msg(LIGHT_TOPIC, color)
    emotion, action_name = Live2DActions.hair_back2()
    color_name = {"red": "红色", "green": "绿色", "blue": "蓝色"}[color]
    emoji = {"red": "🔴", "green": "🟢", "blue": "🔵"}[color]
    return (f"灯已打开，当前颜色：{color_name} {emoji}", emotion, action_name, color)


# ────────────────── 工具执行（豆包 tool calling 入口） ──────────────────

async def execute_tool(tool_name: str, tool_args: dict):
    """执行豆包发起的工具调用。返回 (result_text, emotion, action, light_state)"""
    if tool_name == "control_light":
        action = tool_args.get("action", "on")
        color = tool_args.get("color", "red")
        return await control_light(action, color)
    logger.warning(f"未知工具调用: {tool_name}")
    return (f"不支持的工具: {tool_name}", None, None, None)
