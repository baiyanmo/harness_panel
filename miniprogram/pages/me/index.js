Page({
  data: {
    version: '1.0.0',
    features: [
      { icon: '🤖', name: 'AI 管家', desc: '贾维斯智能对话' },
      { icon: '🌤', name: '天气查询', desc: '实时天气播报' },
      { icon: '💡', name: '灯光控制', desc: '巴法云 IoT 控制' },
      { icon: '⏰', name: '智能报时', desc: '语音报时提醒' },
    ],
  },

  goLight() {
    wx.navigateTo({ url: '/pages/light/index' });
  },

  goChat() {
    wx.switchTab({ url: '/pages/chat/index' });
  },
});
