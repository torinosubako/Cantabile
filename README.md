# Cantabile
## About This Repository
#### (JP)
無線センサー・ネットワークシステム『Cantabile』と、それを構成するセンサノード『Sonorous』と中継・処理ノード『Andante』、それを制御するソフトウェア及びクラウド上で動作するプログラム一式<br>
日本語版Readmeはここ >>>
#### (EN)
The wireless sensor network system "Cantabile", its constituent sensor nodes "Sonorous" and relay/processing node "Andante", software to control them and a set of programs running on the cloud.<br>
Click here for the English version of the readme. >>> [Readme_EN](README_en.md)
## About Cantabile
Cantabileは、IoT専用に開発された高価で入手性の低い専用マイコンや、それによって提供される専用通信規格を用いずに、汎用の無線通信機能付きマイコンや汎用通信規格であるBluetooth Low EnergyやWi-Fiを用いてワイヤレスセンサーネットワークを構成し、多機能な二酸化炭素測定・通知デバイスを開発・試作を行う研究のプロジェクト名、及びその研究で試作したワイヤレスセンサーネットワークのことです。
## Cantabile Goals
Cantabileの目標は、IoT専用に開発された高価で入手性の低い専用マイコンや、それによって提供される専用通信規格を用いずに、汎用の無線通信機能付きマイコンや汎用通信規格を用いて、消費電力、ストレージ、プロセッサ等の厳しい制約の中で、使用するリソースとコストを最小限に抑え、ある程度以上の信頼性を維持しながら、比較的安価かつ高精度で高い長期運用安定性を有するセンサー・ネットワークシステムを開発することです。このシステムの開発から得られた知見・経験及び生じた問題と、その解決策等は、他の無線アプリケーションやネットワークにも応用できると考えられます。

## システム構成


## 各プログラム及びフォルダの構成と概要
### センサーノード開発系
* Sonorous - 初期型センサノード(独立電源タイプ)<br> 
    主な構成
    * マイコン : M5StickC Plus(ESP32-PICO-D4)
    * 温湿度センサ : SHT30
    * 大気圧センサ : BMP280
    * 二酸化炭素センサ方式 : NDIR
    * 二酸化炭素センサ : MH-Z19C
    * 電源 : 独立電源
    
    2種類作成した初期型ノードの１つ、要点は2つ
    1. 一部の省電力実装のみ実施(論文4.4①相当)
    1. そもそも独立電源タイプを常時電源駆動させても変わらない為、開発を独立電源タイプに一本化した。

* Sonorous_Atom - 初期型センサノード(常時電源タイプ)<br> 
    主な構成<br>
    * マイコン : M5ATOM Lite(ESP32-PICO-D4)
    * 温湿度センサ : SHT30
    * 大気圧センサ : BMP280
    * 二酸化炭素センサ方式 : NDIR
    * 二酸化炭素センサ : MH-Z19C
    * 電源 : 常時電源
    
    2種類作成した初期型ノードのもう片方、要点は4つ
    1. 一部の省電力実装のみ実施(論文4.4①相当)
    1. 常時電源駆動を前提にハードウェアをATOM Liteに変更
    1. 独立電源タイプの受電圧モニタリング機能等をオミット(HW上にそもそも電源管理ICがない)
    1. そもそも独立電源タイプを常時電源駆動させても変わらない為、開発を独立電源タイプに一本化した。

* Sonorous_Plus - 改良型センサノード(独立電源タイプ)<br> 
    主な構成<br>
    * マイコン : M5StickC Plus(ESP32-PICO-D4)
    * 温湿度センサ : BME680
    * 大気圧センサ : BME680
    * 二酸化炭素センサ方式 : NDIR
    * 二酸化炭素センサ : MH-Z19C
    * 電源 : 独立電源
    
    初期型(独立電源)ノードの省電力改良タイプ、要点は3つ
    1. MH-Z19Cの間欠動作等の省電力実装のみ実施(論文4.4②相当)
    1. 電源変動等によるデータ成型不具合を修正
    1. 基板設計②に対応し、BME680に温湿度・大気圧センサーを換装

* Sonorous_Plus_alpha - 改良型センサノードKai(センサー換装・実装最適化)<br> 
    主な構成<br>
    * マイコン : M5StickC Plus(ESP32-PICO-D4)
    * 温湿度センサ : SCD41
    * 大気圧センサ : DPS310
    * 二酸化炭素センサ方式 : 光音響効果
    * 二酸化炭素センサ : SCD41
    * 電源 : 独立電源
    
    改良型センサノードをベースに、CO2センサーを低消費電力で高精度のSCD41に換装
    1. 全ての省電力技術を実装(論文4.4③相当)
    1. 低消費電力化を目指し、二酸化炭素センサーをSCD41に変更
    1. 温湿度センサーをBME680から、SCD41へ統合・変更
    1. 大気圧センサーをBME680から、非常に高精度なDPS310へ変更
    1. ノード間伝送時のデータ成型不具合の修正

### 中継・処理ノード開発系
* Andante - 技術実証用・最初期型中継・処理ノード<br> 
    構成
    * マイコン : M5Paper(ESP32-D0WDQ6-V3)
    
    技術実証用に試作した最初期型中継処理ノード。開発目的は主に2つ
    1. BLEによるノード間通信技術の確立・検証、またBLEそのものの動作パターンの検証
    1. ノード間通信のデータセット開発

* Andante_Yoko - 初期型実用中継・処理ノード<br> 
    構成
    * マイコン : M5Paper(ESP32-D0WDQ6-V3)
    
    実用的な初期型中継処理ノード。この時点でAWS連携以外の基本的な機能の実装は終わっている。
    1. ノード間通信により受信した情報の外部転送機能の実装(中継ノード->Ambient)
    1. 外部情報受信機能の実装(ODPT->中継ノード)
    1. ノード間通信により受信した情報を元にアラートを発し表示を変化させるアラート機能の実装
    1. これらを支えるGFXライブラリの導入

* Andante_Yoko_AWS - AWS連携対応型中継・処理ノード<br> 
    構成
    * マイコン : M5Paper(ESP32-D0WDQ6-V3)
    
    AWSに連携する事に対応した中継処理ノード。AWS連携試験等の成果を反映している。
    1. ノード間通信により受信した情報のAWS連携・転送機能の実装(中継ノード->AWS)
    1. AWS接続の為のMQTT通信・SSL/TLS通信対応化
    1. センサーノード開発で培われた省電力制御技術の一部導入
    1. 複雑化する機能と関係ライブラリを見据えた軽量化実装の実施

* Andante_Yoko_AWS_Kai - 改良型AWS連携対応中継・処理ノード(制御最適化)<br> 
    構成
    * マイコン : M5Paper(ESP32-D0WDQ6-V3)
    
    Andante_Yoko_AWSの動作結果を反映し、制御系の最適化により高い信頼性と可用性を目指した中継処理ノード。
    1. 安定化の為に制御思想の刷新
    1. 制御フローの単純化と制御ロジックの刷新
    1. ソフトウェアタイマーを用いた固定制御方式から、ソフトウェアタイマーとRTCタイマーを組み合わせたアダプティブなタイマー制御を有する複合時素式制御へ変更
    1. ノード間通信の受信機会向上を意図したチューニングを実施
