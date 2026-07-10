import './hotelSidebar.css'
import { fetchWeather, controlLight, fetchLightStatus, type WeatherData } from '../api/client'

export interface SidebarCallbacks {
  /** 灯光颜色变化 'red' | 'green' | 'blue' | 'off' */
  onLightChange: (color: string) => void
  /** 天气数据更新 */
  onWeatherUpdate?: (data: WeatherData) => void
}

export class HotelSidebar {
  readonly element: HTMLDivElement

  private _weatherCity = '邯郸'
  private _refreshTimer: ReturnType<typeof setInterval> | null = null
  private _lightPollTimer: ReturnType<typeof setInterval> | null = null

  // 子元素引用
  private _weatherCityEl!: HTMLElement
  private _weatherTempEl!: HTMLElement
  private _weatherDescEl!: HTMLElement
  private _weatherHumidityEl!: HTMLElement
  private _weatherWindEl!: HTMLElement
  private _lightLabelEl!: HTMLElement
  private _lightBtns!: NodeListOf<HTMLElement>

  // 当前灯光颜色
  private _lightColor: string = 'off'

  constructor(private callbacks: SidebarCallbacks) {
    const el = (this.element = document.createElement('div'))
    el.id = 'hotel-sidebar'
    el.innerHTML = `
      <!-- 天气 -->
      <div class="panel weather-panel">
        <div class="panel-title"> 天气</div>
        <div class="weather-main">
          <div class="weather-temp" id="hs-temp">--°</div>
          <div>
            <div class="weather-desc" id="hs-desc">加载中...</div>
            <div class="weather-city" id="hs-city">📍 邯郸</div>
          </div>
        </div>
        <div class="weather-extra">
          <span>💧 湿度 <b id="hs-humidity">--</b></span>
          <span>🌬 风速 <b id="hs-wind">--</b></span>
        </div>
      </div>

      <!-- 住房 -->
      <div class="panel room-panel">
        <div class="panel-title room-title" id="hs-room-panel-title">C207</div>
        <div class="room-stats">
          <div class="room-stat occupied">
            <div class="stat-dot red"></div>
            <div class="stat-label">已入住</div>
          </div>
          <div class="room-stat available">
            <div class="stat-dot green off"></div>
            <div class="stat-label">未入住</div>
          </div>
          <div class="room-stat cleaning">
            <div class="stat-dot yellow off"></div>
            <div class="stat-label">待打扫</div>
          </div>
        </div>
      </div>

      <!-- 灯光 -->
      <div class="panel light-panel">
        <div class="panel-title">💡 灯光控制</div>
        <div class="light-color-btns" id="hs-light-btns">
          <button class="light-btn" data-color="red">🔴 红灯</button>
          <button class="light-btn" data-color="green">🟢 绿灯</button>
          <button class="light-btn" data-color="blue">🔵 蓝灯</button>
          <button class="light-btn active" data-color="off">⚫ 关灯</button>
        </div>
        <div class="light-label">
          <span id="hs-light-label">⚫ 灯已关闭</span>
        </div>
      </div>
    `

    // 缓存引用
    this._weatherCityEl = el.querySelector('#hs-city')!
    this._weatherTempEl = el.querySelector('#hs-temp')!
    this._weatherDescEl = el.querySelector('#hs-desc')!
    this._weatherHumidityEl = el.querySelector('#hs-humidity')!
    this._weatherWindEl = el.querySelector('#hs-wind')!
    this._lightBtns = el.querySelectorAll('#hs-light-btns .light-btn')
    this._lightLabelEl = el.querySelector('#hs-light-label')!

    this._bindEvents()
  }

  private _bindEvents() {
    this._lightBtns.forEach(btn => {
      btn.addEventListener('click', () => {
        const color = btn.dataset.color || 'off'
        this.setLightColor(color)
      })
    })
  }

  /** 更新按钮 UI 状态 */
  private _updateLightBtns(color: string) {
    this._lightBtns.forEach(b => {
      b.classList.toggle('active', b.dataset.color === color)
    })
    const labels: Record<string, string> = {
      red: '🔴 红灯',
      green: '🟢 绿灯',
      blue: '🔵 蓝灯',
      off: '⚫ 灯已关闭',
    }
    this._lightLabelEl.textContent = labels[color] || '⚫ 灯已关闭'
  }

  /** 同步硬件灯光 */
  private _syncLightToDevice(color: string) {
    controlLight(color).catch(err => {
      console.warn('[sidebar] 灯光控制失败:', err)
    })
  }

  /** 设置灯光颜色 */
  setLightColor(color: string) {
    this._lightColor = color
    this._updateLightBtns(color)
    this._syncLightToDevice(color)
    this.callbacks.onLightChange(color)
  }

  get lightColor() {
    return this._lightColor
  }

  /** 根据后端返回的聊天灯光状态设置颜色（不重复请求） */
  applyLightFromChat(color: string) {
    if (color === this._lightColor) return
    this._lightColor = color
    this._updateLightBtns(color)
    // 不调 _syncLightToDevice，因为 chat 已经发过巴法云指令了
  }

  /** 开始加载数据 */
  async loadData() {
    await this._loadWeather()
    // 每 30 分钟刷新天气
    this._refreshTimer = setInterval(() => this._loadWeather(), 30 * 60 * 1000)
    // 每 5 秒轮询灯光状态
    this._pollLightStatus()
    this._lightPollTimer = setInterval(() => this._pollLightStatus(), 5000)
  }

  private async _loadWeather() {
    try {
      const data = await fetchWeather(this._weatherCity)
      this._renderWeather(data)
      this.callbacks.onWeatherUpdate?.(data)
    } catch (err) {
      console.warn('[sidebar] 天气加载失败:', err)
      this._weatherTempEl.textContent = '--°'
      this._weatherDescEl.textContent = '天气不可用'
    }
  }

  private _renderWeather(data: WeatherData) {
    if (data.error) {
      this._weatherTempEl.textContent = '--°'
      this._weatherDescEl.textContent = data.error
      return
    }
    this._weatherCityEl.textContent = `📍 ${data.city}`
    this._weatherTempEl.textContent = `${data.temperature}°`
    this._weatherDescEl.textContent = data.weather_desc
    this._weatherHumidityEl.textContent = `${data.humidity}%`
    this._weatherWindEl.textContent = `${data.wind_speed || '--'}`
  }

  /** 设置天气城市 */
  setWeatherCity(city: string) {
    this._weatherCity = city
    this._loadWeather()
  }

  /** 获取当前天气城市 */
  get weatherCity() {
    return this._weatherCity
  }

  /** 销毁 */
  destroy() {
    if (this._refreshTimer) clearInterval(this._refreshTimer)
    if (this._lightPollTimer) clearInterval(this._lightPollTimer)
  }

  /** 轮询巴法云设备灯光状态并同步 UI */
  private async _pollLightStatus() {
    try {
      const data = await fetchLightStatus()
      const color = data.color
      // 只处理已知颜色，unknown 表示无设备数据
      if (color && color !== 'unknown' && color !== this._lightColor) {
        this._lightColor = color
        this._updateLightBtns(color)
        this.callbacks.onLightChange(color)
      }
    } catch (err) {
      console.warn('[sidebar] 灯光状态轮询失败:', err)
    }
  }
}
