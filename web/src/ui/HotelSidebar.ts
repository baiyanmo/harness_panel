import './hotelSidebar.css'
import { fetchWeather, controlLight, type WeatherData } from '../api/client'

export interface SidebarCallbacks {
  /** 灯光档位变化 0–4 */
  onLightChange: (level: number) => void
  /** 天气数据更新 */
  onWeatherUpdate?: (data: WeatherData) => void
}

export class HotelSidebar {
  readonly element: HTMLDivElement

  private _weatherCity = '邯郸'
  private _lightLevel = 4
  private _refreshTimer: ReturnType<typeof setInterval> | null = null

  // 子元素引用
  private _weatherCityEl!: HTMLElement
  private _weatherTempEl!: HTMLElement
  private _weatherDescEl!: HTMLElement
  private _weatherHumidityEl!: HTMLElement
  private _weatherWindEl!: HTMLElement
  private _lightDots!: NodeListOf<HTMLElement>
  private _lightSlider!: HTMLInputElement
  private _lightLabelEl!: HTMLElement

  // 当前灯光是否已开启（避免重复请求）
  private _lightOn = false

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
        <div class="light-dots" id="hs-light-dots">
          <div class="light-dot" data-level="0"></div>
          <div class="light-dot" data-level="1"></div>
          <div class="light-dot" data-level="2"></div>
          <div class="light-dot" data-level="3"></div>
          <div class="light-dot active" data-level="4"></div>
        </div>
        <input type="range" class="light-slider" id="hs-light-slider" min="0" max="4" value="4" step="1">
        <div class="light-label">
          <span id="hs-light-label">💡 全亮</span>
        </div>
      </div>
    `

    // 缓存引用
    this._weatherCityEl = el.querySelector('#hs-city')!
    this._weatherTempEl = el.querySelector('#hs-temp')!
    this._weatherDescEl = el.querySelector('#hs-desc')!
    this._weatherHumidityEl = el.querySelector('#hs-humidity')!
    this._weatherWindEl = el.querySelector('#hs-wind')!
    this._lightDots = el.querySelectorAll('#hs-light-dots .light-dot')
    this._lightSlider = el.querySelector('#hs-light-slider')!
    this._lightLabelEl = el.querySelector('#hs-light-label')!

    this._bindEvents()
  }

  private _bindEvents() {
    // 灯光圆点点击
    this._lightDots.forEach(dot => {
      dot.addEventListener('click', () => {
        const level = Number(dot.dataset.level)
        this.setLightLevel(level)
      })
    })

    // 灯光滑块——边拖边同步后端
    this._lightSlider.addEventListener('input', () => {
      const level = Number(this._lightSlider.value)
      this._updateLightUI(level)
    })
    this._lightSlider.addEventListener('change', () => {
      const level = Number(this._lightSlider.value)
      this._lightLevel = level
      this._updateLightDots(level)
      this._syncLightToDevice(level)
      this.callbacks.onLightChange(level)
    })
  }

  private _updateLightUI(level: number) {
    // 实时更新圆点和 label（slider input 事件）
    this._updateLightDots(level)
    const labels = ['🌑 全暗', '🌒 微光', '🌓 柔和', '🌖 明亮', '💡 全亮']
    this._lightLabelEl.textContent = labels[level]
  }

  private _updateLightDots(level: number) {
    this._lightDots.forEach(d => {
      d.classList.toggle('active', Number(d.dataset.level) <= level)
    })
  }

  /** 开始加载数据 */
  async loadData() {
    await this._loadWeather()
    // 每 30 分钟刷新天气
    this._refreshTimer = setInterval(() => this._loadWeather(), 30 * 60 * 1000)
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

  /** 根据档位同步硬件灯光：档位 0 → 关灯，其余 → 开灯 */
  private _syncLightToDevice(level: number) {
    const desired = level > 0
    if (desired === this._lightOn) return  // 状态未变，跳过
    this._lightOn = desired
    controlLight(desired).catch(err => {
      console.warn('[sidebar] 灯光控制失败:', err)
    })
  }

  /** 设置灯光档位 */
  setLightLevel(level: number) {
    this._lightLevel = level
    this._lightSlider.value = String(level)
    this._updateLightUI(level)
    this._syncLightToDevice(level)
    this.callbacks.onLightChange(level)
  }

  get lightLevel() {
    return this._lightLevel
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
  }
}
