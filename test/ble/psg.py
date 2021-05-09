import asyncio
import time
import numpy as np

import PySimpleGUI as sg

sg.theme('Dark Blue 3')
layout = [
  [sg.T('Miraisense test GUI')],
  [sg.T('BLE',key='BLE',background_color='#333333'),sg.T('HAP'),sg.T('IMU'),sg.T('BAT'),sg.T('---')],
  [sg.Slider(range=(0, 255), orientation='v', size=(5, 20), default_value=0),sg.Slider(range=(-100, 100), orientation='h', size=(20, 20), default_value=0)],
  [sg.B('TEST'),sg.Exit()]
]

window = sg.Window(title='Controller', size=(200, 200)).Layout(layout)

pvalues=""
async def evloop():
  global pvalues
  while True:
    event, values=window.read(timeout=50)
    if event!='__TIMEOUT__': print("event is ",event)
    if event==sg.WIN_CLOSED or event=='Exit':
      queue.put_nowait('Quit')
      break
    elif event=='TEST':
      window['BLE'].update(background_color='#00FFFF')
    if str(values)!=pvalues:
      print(values,pvalues)
      r=values[0]
      if r>0:
        y=r*values[1]/100
        a=np.arcsin(-y/r)*180/3.1415
        queue.put_nowait('SF I'+str(int(r))+' C'+str(int(a)))
      else:
        queue.put_nowait('SF I0')
      pvalues=str(values)
    await asyncio.sleep(0.05)

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
    window['BLE'].update(background_color='#00FFFF')
    while True:
        cmd = await queue.get()
        print('CMD:',cmd)
        if cmd=='Quit': break
        await client.write_gatt_char(CWR_UUID,cmd.encode('utf-8'))
        queue.task_done()

async def main():
  global queue
  queue=asyncio.Queue()
  await queue.join()
  task1= asyncio.create_task(bl_connect('SPRESENSE'))
  task2= asyncio.create_task(evloop())
  await task1
  await task2

asyncio.run(main())
