import asyncio
import time

import PySimpleGUI as sg

sg.theme('Dark Blue 3')
layout = [
    [sg.Text('Python GUI')],
    [sg.Text('大きさ', size=(15, 1)), sg.InputText('0')],
    [sg.Submit(button_text='Send test')],
    [sg.Submit(button_text='Quit')]
]

window = sg.Window(title='show input text', size=(200, 200)).Layout(layout)

async def evloop():
  while True:
    event, values = window.read()
    print("event is ",event)
    if event == 'Send test':
      queue.put_nowait(b'SF I100 C120')
    elif event == 'Quit':
      queue.put_nowait('Quit')
    await asyncio.sleep(0.1)

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
        if cmd is 'Quit': break
        await client.write_gatt_char(CWR_UUID,cmd)
        queue.task_done()
    print("Connected")

async def main():
  global queue
  queue=asyncio.Queue()
  await queue.join()
  task1= asyncio.create_task(bl_connect('SPRESENSE'))
  task2= asyncio.create_task(evloop())
  await task1
  await task2
  root.update()

asyncio.run(main())
