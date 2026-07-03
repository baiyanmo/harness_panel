"""FastAPI 入口"""
import sys
from pathlib import Path

# 支持直接 python main.py 运行：把 backend/ 加入 sys.path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from src.tools import weather
from src.routes import chat
from src.routes import tts
from src.routes import light

app = FastAPI(title="智慧酒店 API")

# CORS 允许前端跨域
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# 挂载路由
app.include_router(weather.router, prefix="/api")
app.include_router(chat.router, prefix="/api")
app.include_router(tts.router, prefix="/api")
app.include_router(light.router, prefix="/api")


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("src.main:app", host="127.0.0.1", port=8001, reload=True)
    
