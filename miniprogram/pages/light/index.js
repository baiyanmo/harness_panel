const { request } = require('../../utils/request');

Page({
  data: {
    level: 0,         // 0-4 档位
    lightOn: false,
    controlling: false,
    levels: [
      { lv: 0, label: '全暗', icon: '🌑' },
      { lv: 1, label: '微光', icon: '🌒' },
      { lv: 2, label: '柔和', icon: '🌓' },
      { lv: 3, label: '明亮', icon: '🌔' },
      { lv: 4, label: '全亮', icon: '🌕' },
    ],
  },

  onShow() {
    // 读取上次保存的档位
    const saved = wx.getStorageSync('light_level');
    if (saved !== '') {
      this.setData({ level: Number(saved), lightOn: Number(saved) > 0 });
    }
  },

  async onLevelTap(e) {
    const lv = Number(e.currentTarget.dataset.lv);
    if (lv === this.data.level || this.data.controlling) return;

    const isOn = lv > 0;
    this.setData({ controlling: true });

    try {
      await request({
        url: '/light/control',
        method: 'POST',
        data: { state: isOn },
      });
      this.setData({ level: lv, lightOn: isOn });
      wx.setStorageSync('light_level', lv);
    } catch (err) {
      wx.showToast({ title: err.message || '控制失败', icon: 'none' });
    } finally {
      this.setData({ controlling: false });
    }
  },
});
