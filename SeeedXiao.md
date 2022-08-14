# Getting Started
**Seeed Xiao BLE**は**Arduino Nano 33 BLE**と同じSoC(Nordic nRF52840)を搭載したボードです。Nano33との互換性は高、サイズはかなり小なので、プロダクションモデルに近いPoCを実現します。Nano33は**mbedOS**で動いているので、こちらもmbedOSの環境を整えます。
## ボードの追加
1. ファイル=>環境設定=>追加のボードマネージャURLに以下指定
~~~
https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
~~~
2. ツール=>ボード=>ボードマネージャを開き、**seeed**をサーチ。見つかったものから以下の2つをインスト。
    - Seeed nRF52 Boards
    - Seeed nRF52 mbed enabled Boards
## ボードとシリアルポートの選択
1. ツール=>ボード=>Seeed nRF52 mbed enabled Boards、からターゲットボードを選択
2. ツール=>シリアルポート、からボードの接続されているポートを選択

## トラブルシュート
1. ツール=>シリアルポートが消滅
    - 原因：XiaoのBootloaderの性質?
    - 対策：リセットボタンをダブルクリックする
2. ビルドエラー／This code is intended to run on the MBED nRF52840 platform...
    - 原因：ファイルシステムライブラリFS_Nano33BLEでArduino以外のターゲットボードをエラーとしている
    - 対策：Arduino=>libraries=>FS_Nano33BLE.hの以下の行をコメントアウトする
~~~
#if !( defined(ARDUINO_ARCH_NRF52840) && defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARDUINO_NANO33BLE) )
  #error This code is intended to run on the MBED nRF52840 platform! Please check your Tools->Board setting.
#endif
~~~
