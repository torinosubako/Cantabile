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
