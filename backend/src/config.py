"""项目配置 — 从 .env 读取"""
import os
from dotenv import load_dotenv

load_dotenv()  # 读取 backend/.env

AMAP_WEATHER_KEY = os.getenv("AMAP_KEY", "")
DOUBAO_CHAT_KEY = os.getenv("DOUBAO_KEY", "")
TTS_KEY = os.getenv("TTS_KEY", "")
THINGSBOARD_URL = os.getenv("THINGSBOARD_URL", "")
THINGSBOARD_API_KEY = os.getenv("THINGSBOARD_API_KEY", "")
THINGSBOARD_DEVICE_ID = os.getenv("THINGSBOARD_DEVICE_ID", "")
BAFA_SEND_URL = os.getenv("BAFA_SEND_URL", "")
BAFA_GET_URL = os.getenv("BAFA_GET_URL", "")
BAFA_KEY = os.getenv("BAFA_KEY", "")
