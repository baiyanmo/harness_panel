const { request } = require('../../utils/request');

Page({
  data: {
    message: '',
    messages: [],       // UI 显示用：{role, content, time}
    isRecording: false,
    recordingText: '按住说话',
  },

  // 和后端对齐的消息历史
  chatHistory: [],

  onLoad() {
    this.addMessage('ai', '欢迎来到智慧酒店！我是您的 AI 管家贾维斯，有什么可以帮您的吗？');
  },

  onInput(e) {
    this.setData({ message: e.detail.value });
  },

  // 发送消息（核心逻辑）
  async onSend() {
    const text = this.data.message.trim();
    if (!text) return;

    this.addMessage('user', text);
    this.setData({ message: '' });

    // 追加到和后端对齐的历史
    this.chatHistory.push({ role: 'user', content: text });

    try {
      const res = await request({
        url: '/chat',
        method: 'POST',
        data: {
          messages: this.chatHistory,
          stream: false,
          max_tokens: 1024,
          temperature: 0.7,
        },
      });

      const reply = res.content || '(空回复)';
      this.addMessage('ai', reply);

      // 保存 AI 回复到历史
      this.chatHistory.push({ role: 'assistant', content: reply });
    } catch (e) {
      this.addMessage('ai', '⚠️ 网络异常，请稍后重试');
      wx.showToast({ title: e.message || '请求失败', icon: 'none' });
    }
  },

  // ──── 语音输入 ────
  onStartRecord() {
    this.setData({ isRecording: true, recordingText: '松开结束' });

    const recorder = wx.getRecorderManager();
    this._recorder = recorder;

    recorder.onStop((res) => {
      this.setData({ isRecording: false, recordingText: '按住说话' });
      this._recognizeVoice(res.tempFilePath);
    });

    recorder.onError(() => {
      this.setData({ isRecording: false, recordingText: '按住说话' });
      wx.showToast({ title: '录音失败', icon: 'none' });
    });

    recorder.start({
      duration: 60000,
      sampleRate: 16000,
      format: 'mp3',
    });
  },

  onEndRecord() {
    this.setData({ isRecording: false, recordingText: '按住说话' });
    if (this._recorder) this._recorder.stop();
  },

  // 语音识别（WechatSI 插件）
  _recognizeVoice(filePath) {
    try {
      const plugin = requirePlugin('WechatSI');
      const manager = plugin.getRecordRecognitionManager();
      manager.start({ duration: 60000, lang: 'zh_CN' });

      manager.onRecognize = (result) => {
        this.setData({ message: result });
      };

      manager.onStop = (result) => {
        if (result && result.length > 0) {
          this.setData({ message: result });
          this.onSend();
        } else {
          wx.showToast({ title: '未识别到语音', icon: 'none' });
        }
      };

      manager.onError = () => {
        wx.showToast({ title: '语音识别失败', icon: 'none' });
      };
    } catch {
      wx.showToast({ title: '语音插件不可用', icon: 'none' });
    }
  },

  addMessage(role, content) {
    const messages = [...this.data.messages, {
      role,
      content,
      time: new Date().toLocaleTimeString(),
    }];
    this.setData({ messages });
    this.scrollToBottom();
  },

  scrollToBottom() {
    wx.createSelectorQuery()
      .select('#chatList')
      .boundingClientRect()
      .exec(() => {
        wx.pageScrollTo({ scrollTop: 99999, duration: 300 });
      });
  },
});
