const { request } = require('../../utils/request');

Page({
  data: {
    weather: null,
    loading: true,
  },

  onShow() {
    this.loadWeather();
  },

  async loadWeather() {
    this.setData({ loading: true });
    try {
      const res = await request({ url: '/weather?city=邯郸' });
      this.setData({ weather: res, loading: false });
    } catch {
      this.setData({ loading: false });
    }
  },

  goChat() {
    wx.switchTab({ url: '/pages/chat/index' });
  },

  goLight() {
    wx.navigateTo({ url: '/pages/light/index' });
  },
});
