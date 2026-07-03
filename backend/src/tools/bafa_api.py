"""巴法云 API 通信层 — 通用消息发送与获取"""
import httpx
import logging
from ..config import BAFA_SEND_URL, BAFA_GET_URL, BAFA_KEY

logger = logging.getLogger(__name__)

headers = {
    "Content-Type": "application/json; charset=utf-8",
}


async def send_msg(topic: str, msg: str, msg_type: int = 1) -> bool:
    """向指定主题发送消息

    Args:
        topic: MQTT 主题名称
        msg: 消息内容（如 on/off）
        msg_type: 主题类型 1=MQTT 3=TCP

    Returns:
        True 表示发送成功
    """
    body = {
        "uid": BAFA_KEY,
        "topic": topic,
        "type": msg_type,
        "msg": msg,
    }
    try:
        async with httpx.AsyncClient() as client:
            resp = await client.post(BAFA_SEND_URL, headers=headers, json=body, timeout=10)
            data = resp.json()
            if data.get("code") == 0:
                logger.info(f"巴法云发送成功: topic={topic}, msg={msg}")
                return True
            else:
                logger.error(f"巴法云返回异常: {data}")
                return False
    except Exception as e:
        logger.exception(f"巴法云请求失败: {e}")
        return False


async def get_msg(topic: str) -> list[dict]:
    """获取指定主题的最新消息（设备上报数据）

    Args:
        topic: MQTT 主题名称

    Returns:
        消息列表，失败时返回空列表
    """
    params = {
        "uid": BAFA_KEY,
        "topic": topic,
        "type": 1,
    }
    try:
        async with httpx.AsyncClient() as client:
            resp = await client.get(BAFA_GET_URL, params=params, timeout=10)
            data = resp.json()
            if data.get("code") == 0:
                msgs = data.get("data", [])
                logger.info(f"巴法云获取成功: topic={topic}, 消息数={len(msgs)}")
                return msgs
            else:
                logger.error(f"巴法云获取异常: {data}")
                return []
    except Exception as e:
        logger.exception(f"巴法云请求失败: {e}")
        return []