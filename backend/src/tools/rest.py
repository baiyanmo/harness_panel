"""ThingsBoard REST API 通信层"""
import logging
from tb_rest_client.rest_client_ce import RestClientCE
from tb_rest_client.rest import ApiException
from tb_rest_client.models.models_ce import DeviceId
from ..config import THINGSBOARD_URL, THINGSBOARD_API_KEY, THINGSBOARD_DEVICE_ID

logger = logging.getLogger(__name__)


def send_device_attribute(key: str, value: str) -> bool:
    """向 ThingsBoard 设备发送共享属性"""
    if not THINGSBOARD_URL or not THINGSBOARD_API_KEY or not THINGSBOARD_DEVICE_ID:
        logger.error("ThingsBoard 配置不完整")
        return False
    with RestClientCE(base_url=THINGSBOARD_URL) as rest_client:
        try:
            rest_client.api_key_login(api_key=THINGSBOARD_API_KEY)
            target_device = DeviceId(THINGSBOARD_DEVICE_ID, 'DEVICE')
            rest_client.save_device_attributes(target_device, 'SHARED_SCOPE', {key: value})
            return True
        except ApiException as e:
            logger.exception(f"ThingsBoard API Error: {e}")
            return False
