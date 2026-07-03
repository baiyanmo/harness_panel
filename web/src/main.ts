import { Application } from '@pixi/app'
import { InteractionManager } from '@pixi/interaction'
import { extensions } from '@pixi/extensions'
import { Ticker, TickerPlugin } from '@pixi/ticker'
import { Live2DModel } from 'pixi-live2d-display/cubism4'
import { setupZipLoader, loadDefaultModel, replaceModel } from './model/loader'
import { fitToScreen } from './model/config'
import { HotelSidebar } from './ui/HotelSidebar'
import { ChatDialog } from './ui/ChatDialog'

// ---- 一次性初始化 ----
setupZipLoader()
Live2DModel.registerTicker(Ticker)
extensions.add(TickerPlugin, InteractionManager)

let currentKeyHandler: ((e: KeyboardEvent) => void) | null = null

void main()

async function main() {
  const infoEl = document.getElementById('info')!
  const container = document.getElementById('live2d-container')!
  const lightOverlay = document.getElementById('light-overlay')!

  // ---- PixiJS Application (嵌入 #live2d-container) ----
  const app = new Application({
    width: container.clientWidth,
    height: container.clientHeight,
    backgroundColor: 0x1a1a2e, // 暂不需要背景图片
    resolution: window.devicePixelRatio || 2,
    autoDensity: true,
  })

  const canvas = app.view as HTMLCanvasElement
  canvas.style.width = '100%'
  canvas.style.height = '100%'
  canvas.style.display = 'block'
  container.insertBefore(canvas, container.firstChild)

  infoEl.textContent = '⏳ 加载模型中...'

  // ---- 加载默认模型 ----
  let model: Live2DModel
  try {
    model = await loadDefaultModel(app)
    infoEl.textContent = '🖱 拖拽移动 | 滚轮缩放 | 点击互动 | 1-9 动作 Q-P 表情 | Shift 第2页'
  } catch (err) {
    console.error('[live2d] error:', err)
    infoEl.textContent = '❌ 模型加载失败: ' + String(err)
    return
  }

  // ---- 左侧栏 ----
  const sidebar = new HotelSidebar({
    onLightChange(level) {
      const opacities = [0.65, 0.45, 0.28, 0.12, 0.0]
      lightOverlay.style.opacity = String(opacities[level] ?? 0)
    },
  })
  document.body.appendChild(sidebar.element)
  sidebar.loadData()

  // ---- 聊天对话框 ----
  const chat = new ChatDialog({
    onEmotion(name: string) {
      try { model.expression(name) } catch (_) {}
    },
    onAction(name: string) {
      if (name === 'idle') return
      try { model.motion(name) } catch (_) {}
    },
    onWeatherData(data) {
      sidebar.setWeatherCity(data.city)
    },
    onLightState(on: boolean) {
      // 灯光开启 → 档位 4（全亮），关闭 → 档位 0（全暗）
      sidebar.setLightLevel(on ? 4 : 0)
    },
  })
  document.body.appendChild(chat.element)

  // ---- 键盘控制 ----
  setupKeyboard(model, infoEl)

  // ---- 滚轮缩放 ----
  canvas.addEventListener('wheel', (e) => {
    e.preventDefault()
    const delta = e.deltaY > 0 ? -0.05 : 0.05
    const newScale = Math.max(0.1, Math.min(3.0, model.scale.x + delta))
    model.scale.set(newScale)
  }, { passive: false })

  // ---- 全窗口拖放导入 ----
  window.addEventListener('dragover', (e) => e.preventDefault())
  window.addEventListener('drop', async (e) => {
    e.preventDefault()
    const file = e.dataTransfer?.files?.[0]
    if (file && file.name.endsWith('.zip')) {
      infoEl.textContent = '⏳ 加载新模型中...'
      try {
        model = await replaceModel(app, file)
        setupKeyboard(model, infoEl)
        infoEl.textContent = '✨ 新模型已就绪'
      } catch (err) {
        console.error('[live2d] import error:', err)
        infoEl.textContent = '❌ 导入失败: ' + String(err)
      }
    }
  })

  // ---- Responsive: Canvas 跟随容器 ----
  const ro = new ResizeObserver(() => {
    const w = container.clientWidth
    const h = container.clientHeight
    if (w > 0 && h > 0) {
      app.renderer.resize(w, h)
    }
  })
  ro.observe(container)
}

// ---- 键盘控制（从旧 main.ts 保留） ----

function setupKeyboard(model: Live2DModel, infoEl: HTMLElement) {
  if (currentKeyHandler) {
    window.removeEventListener('keydown', currentKeyHandler)
    currentKeyHandler = null
  }

  const mgr = (model as any).internalModel?.motionManager
  if (!mgr?.definitions) return

  const motionList: { group: string; index?: number }[] = []
  for (const group of Object.keys(mgr.definitions)) {
    if (group.toLowerCase() === 'idle') continue
    const defs = mgr.definitions[group] ?? []
    if (defs.length === 1) {
      motionList.push({ group })
    } else {
      defs.forEach((_: any, i: number) => motionList.push({ group, index: i }))
    }
  }

  const exprMgr = mgr.expressionManager
  const exprList: string[] = exprMgr
    ? exprMgr.definitions.map((d: any) => d.Name)
    : []

  const handler = (e: KeyboardEvent) => {
    const target = e.target as HTMLElement
    if (
      target.tagName === 'INPUT' ||
      target.tagName === 'TEXTAREA' ||
      target.tagName === 'SELECT' ||
      (target as any).isContentEditable
    ) {
      return
    }
    if (e.ctrlKey || e.metaKey || e.altKey) return

    if (e.key >= '1' && e.key <= '9') {
      const idx = Number(e.key) - 1
      // 按键 3-9 映射溢出表情 (第21~27个)
      if (idx >= 2 && idx - 2 + 20 < exprList.length) {
        model.expression(exprList[idx - 2 + 20])
        infoEl.textContent = `😊 表情: ${exprList[idx - 2 + 20]}`
        return
      }
      if (idx < motionList.length) {
        const { group, index } = motionList[idx]
        model.motion(group, index)
        infoEl.textContent = `🎬 动作: ${group}${index !== undefined ? `[${index}]` : ''}`
      }
      return
    }

    const exprIdx = 'qwertyuiop'.indexOf(e.key.toLowerCase())
    const offset = e.shiftKey ? 10 : 0
    if (exprIdx >= 0 && exprIdx + offset < exprList.length) {
      model.expression(exprList[exprIdx + offset])
      infoEl.textContent = `😊 表情: ${exprList[exprIdx + offset]}`
      return
    }

    if (e.key === '0') {
      mgr.stopAllMotions()
      exprMgr?.resetExpression()
      infoEl.textContent = '🔄 已重置'
      return
    }
  }

  currentKeyHandler = handler
  window.addEventListener('keydown', handler)
}
