import './chatDialog.css'
import { sendChat, fetchTTS, type WeatherData } from '../api/client'

export interface ChatCallbacks {
  /** Live2D 表情 */
  onEmotion?: (name: string) => void
  /** Live2D 动作 */
  onAction?: (name: string) => void
  /** 天气更新 */
  onWeatherData?: (data: WeatherData) => void
  /** 灯光颜色 'red' | 'green' | 'blue' | 'off' */
  onLightState?: (color: string) => void
}

export class ChatDialog {
  readonly element: HTMLDivElement

  private _sessionId: string
  private _messagesEl!: HTMLElement
  private _inputEl!: HTMLInputElement
  private _sendBtn!: HTMLButtonElement
  private _voiceBtn!: HTMLButtonElement
  private _ttsToggleBtn!: HTMLButtonElement
  private _sending = false
  private _recording = false
  private _autoSpeak = true  // 朗读模式：true 自动朗读，false 不读
  private _currentAudio: HTMLAudioElement | null = null

  constructor(private callbacks: ChatCallbacks = {}) {
    this._sessionId = this._genSessionId()

    const el = (this.element = document.createElement('div'))
    el.id = 'chat-dialog'
    el.innerHTML = `
      <div class="chat-header">
        <span class="online-dot"></span>
        💬 AI 管家
      </div>
      <div class="chat-messages" id="cd-messages">
        <div class="msg ai">
          <div class="msg-label">AI 管家</div>
          欢迎光临智慧酒店！我是您的 AI 管家，请问有什么可以帮您的？😊
        </div>
      </div>
      <div class="chat-input-area">
        <button class="btn-voice" id="cd-voice" title="按住说话">🎤</button>
        <button class="btn-tts-toggle" id="cd-tts-toggle" title="朗读模式：开">🔊</button>
        <input type="text" id="cd-input" placeholder="输入消息..." autocomplete="off">
        <button id="cd-send">📨 发送</button>
      </div>
    `

    this._messagesEl = el.querySelector('#cd-messages')!
    this._inputEl = el.querySelector('#cd-input')!
    this._sendBtn = el.querySelector('#cd-send')!
    this._voiceBtn = el.querySelector('#cd-voice')!
    this._ttsToggleBtn = el.querySelector('#cd-tts-toggle')!

    this._bindEvents()
  }

  private _genSessionId(): string {
    return 'web_' + Math.random().toString(36).slice(2, 10)
  }

  private _recognition: any = null   // SpeechRecognition 实例
  private _voiceTimer: any = null    // max 15s 定时器
  private _speakingMsgEl: HTMLElement | null = null  // 正在朗读的消息元素

  private _bindEvents() {
    this._sendBtn.addEventListener('click', () => this._send())
    this._inputEl.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') this._send()
    })
    // 按住说话 — 鼠标
    this._voiceBtn.addEventListener('mousedown', (e) => { e.preventDefault(); this._startRecording() })
    this._voiceBtn.addEventListener('mouseup', () => this._stopRecording())
    this._voiceBtn.addEventListener('mouseleave', () => this._stopRecording())
    // 按住说话 — 触屏
    this._voiceBtn.addEventListener('touchstart', (e) => { e.preventDefault(); this._startRecording() })
    this._voiceBtn.addEventListener('touchend', (e) => { e.preventDefault(); this._stopRecording() })
    // 消息区的 TTS 播放按钮（事件委托）
    this._messagesEl.addEventListener('click', (e) => {
      const btn = (e.target as HTMLElement).closest?.('.btn-speak') as HTMLElement | null
      if (!btn) return
      const msgEl = btn.closest('.msg.ai') as HTMLElement | null
      if (!msgEl) return
      this._toggleSpeak(msgEl, btn)
    })
    // 朗读模式切换
    this._ttsToggleBtn.addEventListener('click', () => {
      this._autoSpeak = !this._autoSpeak
      if (!this._autoSpeak) {
        if (this._currentAudio) { this._currentAudio.pause(); this._currentAudio = null }
        this._speakingMsgEl = null
        this._updateSpeakBtns(null)
      }
      this._updateTtsToggle()
    })
  }

  private _startRecording() {
    if (this._recording) return
    // 按话筒时停掉正在朗读的音频
    if (this._currentAudio) { this._currentAudio.pause(); this._currentAudio = null }
    this._speakingMsgEl = null
    this._updateSpeakBtns(null)
    const SpeechRecognition =
      (window as any).SpeechRecognition || (window as any).webkitSpeechRecognition
    if (!SpeechRecognition) {
      alert('当前浏览器不支持语音识别，请使用 Chrome 或 Edge')
      return
    }

    const rec = new SpeechRecognition()
    rec.lang = 'zh-CN'
    rec.interimResults = true    // 实时字幕，边说边显示
    rec.maxAlternatives = 1

    let finalText = ''

    rec.onresult = (e: any) => {
      // 拼接所有结果：interim 实时显示，final 攒起来
      let interim = ''
      for (let i = e.resultIndex; i < e.results.length; i++) {
        if (e.results[i].isFinal) {
          finalText += e.results[i][0].transcript
        } else {
          interim += e.results[i][0].transcript
        }
      }
      this._inputEl.value = finalText + interim
    }

    rec.onerror = (e: any) => {
      console.error('[语音] 识别错误:', e.error)
      this._cleanup()
    }

    rec.onend = () => {
      // 识别完自动发送
      this._cleanup()
      const text = this._inputEl.value.trim()
      if (text) this._send()
    }

    rec.start()
    this._recognition = rec
    this._recording = true
    this._voiceBtn.textContent = '⏹'
    this._voiceBtn.classList.add('recording')
    this._inputEl.placeholder = '🎤 正在聆听...'

    // 最长录 15 秒，超时自动停
    this._voiceTimer = setTimeout(() => {
      if (this._recording) this._stopRecording()
    }, 15000)
  }

  private _stopRecording() {
    if (this._recognition) {
      try { this._recognition.stop() } catch (_) {}
    }
    // cleanup 交给 onend 回调
  }

  private _cleanup() {
    if (this._voiceTimer) { clearTimeout(this._voiceTimer); this._voiceTimer = null }
    this._recognition = null
    this._recording = false
    this._voiceBtn.textContent = '🎤'
    this._voiceBtn.classList.remove('recording')
    this._inputEl.placeholder = '输入消息...'
  }

  private async _send() {
    const text = this._inputEl.value.trim()
    if (!text || this._sending) return

    this._sending = true
    this._inputEl.value = ''
    this._sendBtn.disabled = true
    this._sendBtn.textContent = '⏳...'

    // 用户消息
    this._appendMessage('user', text)

    // 显示 AI 输入中
    const thinkingEl = this._appendMessage('ai', '...')
    this._scrollBottom()

    try {
      const data = await sendChat([{ role: 'user', content: text }])

      // 表情 / 动作 / 灯光 — 先于内容渲染，防止 passthrough 时表情被等待
      if (data.emotion) this.callbacks.onEmotion?.(data.emotion)
      if (data.action) this.callbacks.onAction?.(data.action)
      if (data.light !== undefined && data.light !== null) {
        this.callbacks.onLightState?.(data.light)
      }

      // 替换 "..." 为实际回复
      const content = data.content || '(AI 回复为空)'
      thinkingEl.querySelector('.msg-text')!.textContent = content

      // 给 AI 消息加上播放按钮
      const wrap = thinkingEl.querySelector('.msg-content') as HTMLElement
      if (wrap) {
        const btn = document.createElement('button')
        btn.className = 'btn-speak'
        btn.title = '朗读/停止'
        btn.textContent = '🔊'
        wrap.appendChild(btn)
      }

      // 自动朗读 AI 回复（仅在朗读模式下）
      if (this._autoSpeak) this._speak(content, thinkingEl)

      // 天气（如有）
      // if (data.weather_data) this.callbacks.onWeatherData?.(data.weather_data)
    } catch (err) {
      thinkingEl.querySelector('.msg-text')!.textContent =
        '抱歉，AI 服务暂时不可用，请稍后再试 😥'
      console.error('[chat] 发送失败:', err)
    } finally {
      this._sending = false
      this._sendBtn.disabled = false
      this._sendBtn.textContent = '📨 发送'
      this._inputEl.focus()
      this._scrollBottom()
    }
  }

  private _appendMessage(role: 'user' | 'ai', text: string): HTMLElement {
    const div = document.createElement('div')
    div.className = `msg ${role}`
    if (role === 'ai') {
      div.innerHTML = `
        <div class="msg-label">AI 管家</div>
        <div class="msg-content">
          <span class="msg-text">${text}</span>
        </div>
      `
    } else {
      div.innerHTML = `
        <div class="msg-label">我</div>
        <span class="msg-text">${text}</span>
      `
    }
    this._messagesEl.appendChild(div)
    return div
  }

  private _toggleSpeak(msgEl: HTMLElement, btn: HTMLElement) {
    // 如果正在播同一条，停止
    if (this._speakingMsgEl === msgEl) {
      if (this._currentAudio) { this._currentAudio.pause(); this._currentAudio = null }
      this._speakingMsgEl = null
      this._updateSpeakBtns(null)
      return
    }
    // 播新的
    const text = (msgEl.querySelector('.msg-text') as HTMLElement)?.textContent || ''
    if (text) this._speak(text, msgEl)
  }

  /** 朗读文字，调用后端 TTS */
  private _speak(text: string, msgEl?: HTMLElement) {
    if (this._currentAudio) {
      this._currentAudio.pause()
      this._currentAudio = null
    }

    // 过滤 emoji，只读文字
    const clean = text.replace(
      /[\u{1F000}-\u{1FFFF}]|[\u{2600}-\u{27BF}]|[\u{FE00}-\u{FE0F}]|\u{200D}/gu,
      ''
    ).replace(/\s+/g, ' ').trim()
    if (!clean) return

    fetchTTS(clean).then((arrayBuf: ArrayBuffer) => {
      const blob = new Blob([arrayBuf], { type: 'audio/mpeg' })
      const url = URL.createObjectURL(blob)
      const audio = new Audio(url)
      this._currentAudio = audio

      audio.onplay = () => {
        this._speakingMsgEl = msgEl || null
        this._updateSpeakBtns(msgEl || null)
      }
      audio.onended = audio.onerror = () => {
        this._speakingMsgEl = null
        this._updateSpeakBtns(null)
        URL.revokeObjectURL(url)
        this._currentAudio = null
      }

      audio.play()
    }).catch((err: any) => {
      console.error('[TTS] 失败:', err)
    })
  }

  /** 更新所有消息的播放按钮图标 */
  private _updateSpeakBtns(activeEl: HTMLElement | null) {
    this._messagesEl.querySelectorAll('.btn-speak').forEach(btn => {
      const msgEl = (btn as HTMLElement).closest('.msg.ai') as HTMLElement | null
      ;(btn as HTMLElement).textContent = msgEl === activeEl ? '🔇' : '🔊'
    })
  }

  /** 更新朗读模式切换按钮 */
  private _updateTtsToggle() {
    if (this._autoSpeak) {
      this._ttsToggleBtn.textContent = '🔊'
      this._ttsToggleBtn.title = '朗读模式：开（点击关闭）'
      this._ttsToggleBtn.classList.remove('tts-off')
    } else {
      this._ttsToggleBtn.textContent = '🔇'
      this._ttsToggleBtn.title = '朗读模式：关（点击开启）'
      this._ttsToggleBtn.classList.add('tts-off')
    }
  }

  private _scrollBottom() {
    this._messagesEl.scrollTop = this._messagesEl.scrollHeight
  }

  /** 外部添加 AI 消息 */
  addAiMessage(text: string) {
    this._appendMessage('ai', text)
    this._scrollBottom()
  }

  /** 聚焦输入框 */
  focus() {
    this._inputEl.focus()
  }

  /** 销毁 */
  destroy() {
    // 暂无需要清理的资源
  }
}
