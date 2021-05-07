import logging
import asyncio
import time
from bleak import BleakClient
from bleak import discover

#address = "ed:4a:25:16:e3:96"
address = ""
SVC_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
CWR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
async def run(address, loop):
  print("Discovering")
  devices=await discover()
  target=None 
  for d in devices:
    print(d.address,d.name,d.rssi)
    print(d)
    if 'SPRE' in d.name:
      target=d
      break
  if target is None:
    print('SPRESENSE not found')
    return
  print(target.name)
  print("Connecting",target.address)
  async with BleakClient(target.address) as client:
    log = logging.getLogger(__name__)
    log.info(f"Connected: {client.is_connected}")
#    model_number = await client.read_gatt_char(MODEL_NBR_UUID)
#    print("Model Number: {0}".format("".join(map(chr, model_number))))

    print("Pairing")
    paired = await client.pair(protection_level=2)
    log.info(f"Paired: {paired}")

    for n in range(5):
      print("Writing")
#      await client.write_gatt_char(CWR_UUID,b'$18F00115002552553E;')
      await client.write_gatt_char(CWR_UUID,b'SF I100 C120')
      time.sleep(0.1)

loop = asyncio.get_event_loop()
loop.run_until_complete(run(address, loop))