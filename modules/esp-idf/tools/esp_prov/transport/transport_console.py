# SPDX-FileCopyrightText: 2018-2024 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0
#
# type: ignore
from utils import str_to_bytes

from .transport import Transport


class Transport_Console(Transport):

    async def send_data(self, path, data, session_id=0):
        print('Client->Device msg :', path, session_id, str_to_bytes(data).hex())
        try:
            resp = input('Enter device->client msg : ')
        except Exception as err:
            print('error:', err)
            return None
        return bytearray.fromhex(resp).decode('latin-1')
