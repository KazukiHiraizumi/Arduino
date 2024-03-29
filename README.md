# 起動手順  
1. Arduino(Leonardo)にUSBケーブルが接続されていれば、それを外します
2. 電源プラグにコードを差し込み、電源SWをオン
3. Arduino(Leonardo)のLED(L)が点滅開始します。点滅終了で起動完了

# 調整  
## プログラム調整
Arduinoプログラム **wormdrive** のマクロ(#define)にて、以下の調整が出来ます。

|機能|マクロ名|調整方法|
|:----|:----|:----|
|ハンドル回転方向|INVERT|true:右ハンドル<br>false:左ハンドル|
|ギア比(ウォーム:ハンドル)|RATIO|ギア比(ウォームシャフト:ハンドル)を8ビット範囲(0〜255)で設定します。実際のギア比は**RATIO/256**となります。1を設定したい場合は、RATIOを0とします。なお1を超えるギア比は設定出来できません。|

## プログラム変更方法
1. ArduinoIDEのインストール  
プログラムを変更するには、ArduinoIDEをインストールします。ArduinoIDEは[https://www.arduino.cc/en/Main/Software](https://www.arduino.cc/en/Main/Software)からダウンロードします。
2. 環境設定  
ArduinoIDEを立ち上げ、 *ファイル&rarr;環境設定* を開きます。ダイアログの *スケッチブックの保存場所* に**wormdrive**フォルダをコピーします。  
または *スケッチブックの保存場所* の方を変更しても構いません。
3. プログラムを開く  
*ファイル&rarr;スケッチブック* から**wormdrive**を開きます。ボードへの書込は**右矢印アイコン**で出来ます。
4. 書込みに失敗する場合  
    - ボードの選択を *ツール&rarr;ボード* にて確認。**Arduino Leonardo**になっていなければ選びなおす。

# A4988(ステッピングモータドライバ)  
下図のトリマーにて電流調整します。モータ(TS3692)の定格電流は300mAです。  
![A4988](A4988.png)

# BOM

|部品名|個数|仕様・型式|製造者|
|:----|:----|:----|:----|
|ステッピングモータ|1|200div/rev・TS3692N65|多摩川精機|
|ロータリエンコーダ|1|100p/rev・REL18-100BP|アルファ|
|モータマウント|1|3D-Print||
|モータドライバ|1|A4988|Polulu他|
|マイコン|1|Arduino Leonardo|Arduino|
|ドーターボード|1|UB-ARD01|Sunhayato|
|電源プラグ|1|EIAJ-3(メス)||
|電解コンデンサ|1|220&micro;F||

# ESP32 Getting Started
https://dl.espressif.com/dl/package_esp32_index.json
