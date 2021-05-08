import time
import tkinter

import asyncio
import time
from bleak import BleakClient
from bleak import discover

SVC_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
CWR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

async def bl_connect(name):
  print("Discovering")
  devices=await discover()
  target=None 
  for d in devices:
    print(d.address,d.name,d.rssi)
    print(d)
    if name[:4] in d.name:
      target=d
      break
  if target is None:
    print('SPRESENSE not found')
    return
  print("Connecting",target.address)
  async with BleakClient(target.address) as client:
    print("Pairing")
    paired = await client.pair(protection_level=2)
    print("Connected")
    await client.write_gatt_char(CWR_UUID,b'SF I100 C120')
    while True:
        cmd = await queue.get()
        print('CMD:',cmd)
        if str(cmd).startswith('Quit'): break
        await client.write_gatt_char(CWR_UUID,cmd)
        queue.task_done()

def sendcmd():
  queue.put_nowait(b'SF I100 C120')

root = tkinter.Tk()
root.title("demo")
root.geometry("400x400")
btn_connect = tkinter.Button(root, text="Device Control",command=sendcmd)
btn_connect.grid()

n_loop=0
async def tk_update():
  while True:
    root.update()
    await asyncio.sleep(0.1)

async def main():
  global queue
  queue=asyncio.Queue()
  await queue.join()
  task1= asyncio.create_task(bl_connect('SPRESENSE'))
  task2= asyncio.create_task(tk_update())
  await task1
  await task2
  root.update()

asyncio.run(main())
