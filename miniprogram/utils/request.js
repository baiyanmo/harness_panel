const BASE_URL = 'http://localhost:8001';
// 正式环境改为你的云服务器地址
// const BASE_URL = 'https://your-server.com';

/**
 * 封装 wx.request，自动拼接 /api 前缀
 * @param {string} options.url  — 不含 /api 前缀，如 '/weather?city=邯郸'
 * @param {string} options.method
 * @param {object} options.data
 * @returns {Promise<any>}
 */
function request(options) {
  return new Promise((resolve, reject) => {
    wx.request({
      url: `${BASE_URL}/api${options.url}`,
      method: options.method || 'GET',
      data: options.data || {},
      header: {
        'Content-Type': 'application/json',
        ...(options.header || {}),
      },
      success(res) {
        if (res.statusCode >= 200 && res.statusCode < 300) {
          resolve(res.data);
          return;
        }
        reject(new Error(res.data?.detail || res.data?.message || `请求失败(${res.statusCode})`));
      },
      fail(err) {
        reject(new Error(err.errMsg || '网络异常'));
      },
    });
  });
}

module.exports = { BASE_URL, request };
