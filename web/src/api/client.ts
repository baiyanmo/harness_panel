/** 后端 API 调用封装 */

const BASE = '/api'

async function request<T>(url: string, options?: RequestInit): Promise<T> {
  const resp = await fetch(BASE + url, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  })
  if (!resp.ok) {
    const detail = await resp.json().catch(() => ({ detail: resp.statusText }))
    throw new Error(detail.detail || `HTTP ${resp.status}`)
  }
  return resp.json()
}

// --- 天气 ---

export interface WeatherData {
  city: string
  temperature: string
  humidity: string
  weather_desc: string
  wind_speed?: string
  feels_like?: string
  source?: string
  error?: string
}

export function fetchWeather(city = '邯郸'): Promise<WeatherData> {
  return request<WeatherData>(`/weather?city=${encodeURIComponent(city)}`)
}

// --- TTS 语音合成 ---

export function fetchTTS(text: string): Promise<ArrayBuffer> {
  return fetch(BASE + '/tts', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ text }),
  }).then(resp => {
    if (!resp.ok) throw new Error('TTS failed')
    return resp.arrayBuffer()
  })
}

// --- 聊天 ---

export interface ChatResponse {
  role: string
  content: string
  model?: string
  error?: string
  emotion?: string
  action?: string
  light?: boolean
}

export function sendChat(
  messages: Array<{ role: 'user' | 'assistant' | 'system'; content: string }>,
  options?: { stream?: boolean; max_tokens?: number; temperature?: number },
): Promise<ChatResponse> {
  return request<ChatResponse>('/chat', {
    method: 'POST',
    body: JSON.stringify({
      messages,
      stream: options?.stream ?? false,
      max_tokens: options?.max_tokens ?? 1024,
      temperature: options?.temperature ?? 0.7,
    }),
  })
}

// --- 房间 ---

export interface RoomItem {
  id: number
  room_number: string
  type: string
  price: number
  status: string
  floor: number
  description?: string
  image_url?: string
  created_at: string
}

// --- 灯光 ---

export interface LightControlResponse {
  status: string
  state: boolean
}

export function controlLight(state: boolean): Promise<LightControlResponse> {
  return request<LightControlResponse>('/light/control', {
    method: 'POST',
    body: JSON.stringify({ state }),
  })
}

// --- 房间 ---

export function fetchRooms(params?: {
  page?: number
  page_size?: number
  status?: string
  keyword?: string
}): Promise<{ items: RoomItem[]; total: number }> {
  const qs = new URLSearchParams()
  if (params?.page) qs.set('page', String(params.page))
  if (params?.page_size) qs.set('page_size', String(params.page_size))
  if (params?.status) qs.set('status', params.status)
  if (params?.keyword) qs.set('keyword', params.keyword)
  const query = qs.toString()
  return request(`/rooms${query ? '?' + query : ''}`)
}