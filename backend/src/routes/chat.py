"""聊天路由 — POST /api/chat  调用火山方舟（OpenAI 兼容）"""
import re
import httpx
from fastapi import APIRouter
from fastapi.responses import StreamingResponse
from ..config import DOUBAO_CHAT_KEY
from ..schemas.chat import ChatRequest, ChatOut
from ..tools.weather import fetch_weather
from ..tools.patterns import WEATHER, TIME, DATE, SCENERY, LIGHT
from ..tools.actions import Live2DActions
from ..tools.time import now
from ..tools.bafa_api import send_msg

router = APIRouter()

# ────────────────── 工具调用 ──────────────────

#天气询问
async def _tool_weather(user_text: str):
    """调用高德 API 查天气，返回 (文本, 表情, 动作)"""
    city_match = re.search(

  r'(邯郸|北京|上海|广州|深圳|杭州|成都|武汉|西安|南京|重庆)',
    user_text)
    city = city_match.group(1) if city_match else "邯郸"

    w = await fetch_weather(city)

    if w.error:
        emotion, action = Live2DActions.black_face()
        return (f"抱歉，{city}的天气暂时查不到 😥", emotion, action)

    emotion, action = None, None
    temp = float(w.temperature) if w.temperature.lstrip('-').replace('.', '').isdigit() else 0

    # 带伞 / 下雨
    if re.search(r"伞|雨|淋", user_text):
        if "雨" in w.weather_desc:
            return (f"{city}今天{w.weather_desc}，出门记得带伞哦 ☔", emotion, action)
        else:
            return (f"{city}今天是{w.weather_desc}，不会下雨，放心出门吧~", emotion, action)

    #冷热 / 温度 / 穿衣服
    if re.search(r"冷|热|温度|几度|多少度|穿|降温|升温|冷不|热不|外面|出门|闷|潮湿|干燥", user_text):
        if temp <= 0:
            feel = "超冷，出门裹紧羽绒服！🧣"
            emotion, action = Live2DActions.cry()
        elif temp <= 10:
            feel = "挺冷的，穿厚点别感冒 🧥"
            emotion, action = Live2DActions.angry()
        elif temp <= 15:
            feel = "稍微有点凉，带件外套就好"
            emotion, action = Live2DActions.coat()
        elif temp <= 25:
            feel = "温度刚刚好，很舒服 😊"
            emotion, action = Live2DActions.star()
        elif temp <= 32:
            feel = "有点热，穿短袖就行 ☀️"
            emotion, action = Live2DActions.coat()
        else:
            feel = "超级热，注意防晒多喝水 🥵"
            emotion, action = Live2DActions.coat()
        return (f"{city}现在{w.temperature}°C，{w.weather_desc}。{feel}", emotion, action)

    # 雪
    if re.search(r"雪|冰雹", user_text):
        if "雪" in w.weather_desc:
            return (f"{city}今天{w.weather_desc}，赏雪开心，注意路滑哦 ❄️", emotion, action)
        else:
            return (f"{city}今天没有雪，是{w.weather_desc}~", emotion, action)

    # 风
    if re.search(r"风|刮", user_text):
        return (f"{city}今天{w.weather_desc}，风力{w.wind_speed}。"
                f"{'风不小，注意防风 🍃' if '大' in w.wind_speed or int(w.wind_speed[0]) >= 5 else '微风拂面挺舒服~'}", emotion, action)

    # 雾 / 霾 / 空气
    if re.search(r"雾|霾|空气|PM", user_text):
        return (f"{city}目前{w.weather_desc}，湿度{w.humidity}%。"
                f"{'能见度可能不太好，出门注意安全 🚗' if '雾' in w.weather_desc or '霾' in w.weather_desc else '空气还行~'}", emotion, action)

    # 默认完整天气
    return (
        f"{city}实时天气 👀\n"
        f"🌤 {w.weather_desc}  🌡 {w.temperature}°C\n"
        f"💧 湿度 {w.humidity}%  🌬 风力 {w.wind_speed}\n"
        f"数据来源：{w.source}",
        emotion,
        action,
    )

#报时
async def _tool_time(_user_text: str):
    """返回当前时间，含时段动作"""
    n = now()
    h = n.hour
    time_str = f"{h:02d}:{n.minute:02d}:{n.second:02d}"

    if 0 <= h < 6:
        text = f"⏰ 现在是 {time_str}，夜深了，该睡觉啦 😴"
        emotion, action = Live2DActions.doze()
    elif 8 <= h < 9:
        text = f"⏰ 现在是 {time_str}，早餐时间到，去干饭吧！🥐"
        emotion, action = Live2DActions.milk_tea()
    elif 12 <= h < 13:
        text = f"⏰ 现在是 {time_str}，该吃午饭啦 🍚"
        emotion, action = Live2DActions.milk_tea()
    elif 17 <= h < 19:
        text = f"⏰ 现在是 {time_str}，晚餐时间到 🍜"
        emotion, action = Live2DActions.milk_tea()
    else:
        text = f"⏰ 现在是 {time_str}"
        emotion, action = Live2DActions.phone()

    return (text, emotion, action)

#日期
async def _tool_date(_user_text: str):
    """返回当前日期"""
    n = now()
    wd = ["一", "二", "三", "四", "五", "六", "日"][n.weekday()]
    text = f"📅 今天是 {n.year}年{n.month}月{n.day}日 星期{wd}"
    emotion, _ = Live2DActions.phone()
    return (text, emotion, None)

#周边景点 — 只注动作，走豆包回复
async def _tool_scenery(_user_text: str):
    """景点 → 黑发动作，内容走豆包"""
    emotion, _ = Live2DActions.black_hair()
    return (emotion, None)

async def _tool_light(_user_text: str):
    """开灯/关灯 → 巴法云控制 + 回复文本"""
    if re.search(r"开|亮|调大|灯*开", _user_text):
        await send_msg("LIGHT002", "on")
        emotion, action = Live2DActions.hair_back2()
        return ("已经帮您把灯打开了 💡", emotion, action, True)
    elif re.search(r"关|暗|调小|灯*关", _user_text):
        await send_msg("LIGHT002", "off")
        emotion, action = Live2DActions.doze()
        return ("灯已经炸了 💥", emotion, action, False)
    return (None, None, None, None)

TOOLS = [
    {"pattern": WEATHER, "handler": _tool_weather},
    {"pattern": TIME, "handler": _tool_time},
    {"pattern": DATE, "handler": _tool_date},
    {"pattern": LIGHT, "handler": _tool_light},
    {"pattern": SCENERY, "handler": _tool_scenery, "passthrough": True},
]
  
# ────────────────── 调用大模型信息 ──────────────────

ARK_URL = "https://ark.cn-beijing.volces.com/api/v3/chat/completions"
MODEL = "ep-20260607142413-sd26g"

# ────────────────── 系统级提示 ──────────────────

SYSTEM_PROMPT = """你是智慧酒店的 AI 管家，名叫'贾维斯'。
  - 语气亲切专业，称呼用户为'您'。
  - 酒店的地址是：河北省邯郸市丛台区太极路19号。
  - 你只能回答酒店相关的问题，推荐周边景点和美食。
  - 说话简短一点，方式俏皮可爱活泼一点。
  - 不认识的问题诚实说不知道，不要捏造。"""

# ────────────────── 流式生成器 ──────────────────
async def _stream(payload: dict):
    """逐行读取火山方舟 SSE 流，原样转发给前端"""
    async with httpx.AsyncClient(timeout=60.0) as client:
        async with client.stream("POST", ARK_URL, json=payload, headers={
            "Authorization": f"Bearer {DOUBAO_CHAT_KEY}",
            "Content-Type": "application/json",
        }) as resp:
            async for line in resp.aiter_lines():
                if line.startswith("data: "):
                    data_str = line[6:]
                    if data_str == "[DONE]":
                        yield "data: [DONE]\n\n"
                        break
                    yield f"data: {data_str}\n\n"


# ────────────────── 路由 ──────────────────
@router.post("/chat")
async def chat(req: ChatRequest):
    user_text = req.messages[-1].content if req.messages else ""#取对话记录里最新的
    
    # ── 优先匹配本地工具 ──
    emotion, action, light_state = None, None, None
    text = None
    for tool in TOOLS:
        if re.search(tool["pattern"], user_text):
            result = await tool["handler"](user_text)
            if isinstance(result, tuple):
                if len(result) == 2:
                    emotion, action = result
                elif len(result) == 4:
                    text, emotion, action, light_state = result
                else:
                    text, emotion, action = result
            else:
                text = result

            # passthrough: 只注动作，内容走豆包
            if tool.get("passthrough"):
                break

            return ChatOut(content=text or "", emotion=emotion, action=action, light=light_state)

    # 未命中工具： content 可能为 None 时需要设置
    if text is None:
        text = ""
    payload = {
        "model": MODEL,
        "messages": [{"role": "system", "content": SYSTEM_PROMPT}]
                  + [m.model_dump() for m in req.messages],
        "stream": req.stream,
        "max_tokens": req.max_tokens,
        "temperature": req.temperature,
    }

    # 流式：返回 SSE
    if req.stream:
        return StreamingResponse(
            _stream(payload),
            media_type="text/event-stream",
        )

    # 非流式：返回 ChatOut
    try:
        async with httpx.AsyncClient(timeout=30.0) as client:
            resp = await client.post(ARK_URL, json=payload, headers={
                "Authorization": f"Bearer {DOUBAO_CHAT_KEY}",
                "Content-Type": "application/json",
            })
            data = resp.json()
            choice = data["choices"][0]["message"]
            return ChatOut(content=choice["content"], model=data.get("model"),
                           emotion=emotion, action=action)
    except Exception as e:
        return ChatOut(content="", error=str(e),
                       emotion=emotion, action=action)
