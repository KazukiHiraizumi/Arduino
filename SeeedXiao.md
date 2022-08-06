# Getting Started
ArduinoIDEを以下の手順で対応させます。
## ボードの追加
1. ファイル=>環境設定=>追加のボードマネージャURLに以下指定
~~~
https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
~~~
2. ツール=>ボード=>ボードマネージャを開き、**seeed**をサーチ
- Seeed nRF52 Boardsをインストール
- Seeed nRF52 mbed enabled Boards
## ボードとシリアルポートの選択
1. ツール=>ボード=>Seeed nRF52 mbed enabled Boards、からターゲットボードを選択
2. ツール=>シリアルポート、からボードの接続されているポートを選択

## トラブルシュート
1. ツール=>シリアルポートが消滅
 - リセットボタンをダブルクリックする
2. ビルドエラー／adafruit-nrfutilが見つからない
 -
