import type { Application } from '@pixi/app'
import type { Live2DModel } from 'pixi-live2d-display/cubism4'

export const SCALE_MIN = 0.1
export const SCALE_MAX = 3.0
export const SCALE_STEP = 0.05

export function clampScale(s: number): number {
  return Math.max(SCALE_MIN, Math.min(SCALE_MAX, s))
}

/** 缩放模型以适配窗口，返回缩放值 */
export function fitToScreen(app: Application, model: Live2DModel): number {
  const s = Math.min(
    (app.renderer.width * 0.85) / model.internalModel.width,
    (app.renderer.height * 0.85) / model.internalModel.height,
  )
  const clamped = clampScale(s)
  model.scale.set(clamped)
  model.x = app.renderer.width * 0.25
  model.y = app.renderer.height * 0.5
  return clamped
}
