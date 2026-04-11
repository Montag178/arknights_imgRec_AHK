アークナイツの操作を補助するAutoHotkey（AHK）スクリプトです。 ステージの横幅（要塞マップ等）によって変化する「撤退ボタン」および「スキルボタン」の場所を画像認識で探してクリックします。

## **注意事項**

* **自己責任での運用**: 本スクリプトは画像認識によってボタンの位置を探してクリックするものであり、自動周回などの機能は一切ありません。利用の際は[公式の利用規約](https://www.arknights.jp/terms_of_service)を確認し、自己責任で使ってください。  
* オペレーター選択時に出る「白いひし形」を基準にするため、保全駐在や新しく実装されるハディヤなどの傭兵職分ではおそらく機能しません。

## **目的**

エミュレータの固定キーマップでは対応できない、マップの広さによるボタン位置のズレをなくすためのものです。画面を読み取って、正しいボタンの位置を押せるようにします。

## **仕組み**

* **キャプチャ**: Windows Graphics Capture API を使っています。遅延は50ms程度。  
* **解析 (OpenCV)**:  
  1. 画像から線を抜き出します。  
  2. 角度を見て、オペレーター選択時の「白いひし形」の線を特定します。  
  3. その線からボタンの位置を計算します。処理時間は約50msです。

*ひし形の線と、計算されたボタン位置のイメージ。この線の上に他のUIが重なる環境では検出に失敗する。*

* **入力**:  
  * ControlClick で操作します。  
  * LDPlayerなどのエミュレーターではControlClickが不安定な場合があります。その場合は arknights\_imgRec\_adb.ahk を試してみてください。

## **使い方**

1. **ダウンロード**: \[Releases\] からZIPファイルをダウンロードして展開します。  
2. **設定**:  
   * 使うエミュレータのファイル名（.exe）をスクリプトの中で指定してください。  
   * ahkスクリプトを bin フォルダと違う場所に置く場合は、スクリプトの中の SetWorkingDir でパスを指定する必要があります。  
   * arknights\_imgRec\_adb.ahk を使う場合はエミュレーターの解像度を設定してください。  
3. **起動**: ahkファイルを実行します。  
   * ※**AutoHotkey v2 (64-bit)** での起動が必須です。32-bit版では動きません。  
   * ※arknights\_imgRec\_adb.ahk を使う場合はエミュレーターを起動してadb connectしてからスクリプトを起動してください。

デフォルトのキーバインドは以下です。

| キー操作 | 動作 |
| :---- | :---- |
| **Ctrl \+ 1** | スキル発動 |
| **Ctrl \+ 2** | 撤退 |
| **Ctrl \+ 3** | 選択キャンセル |

## **うまく動かないときは**

* test\_functions.ahk で動作を確認してみてください。  
* 動作確認済みの環境：  
  * 解像度: 960x540, 1280x720, 1920x1080  
  * ゲーム内設定：UI調整 0  
* ahkスクリプト内の captureDelay や cancelDelay を調整してみてください。  
* 画像処理の閾値を調整する場合、自力でビルドする必要があります。調整候補：  
  * cv::Scalar lower の値（環境に合わせて240〜255付近）  
  * ROI（関心領域）の大きさ  
  * HoughLinesP の第6, 7引数（検出しやすさに大きく影響します）

## **ビルド方法**

### **必要なもの**

* Visual Studio 2026 (Build Tools または Community)  
* Ninja build  
* VSCode (拡張機能: C/C++, CMake Tools, AHK++)  
* **OpenCV (必須)**: vcpkg でインストールするか、公式サイトからダウンロードしたバイナリが必要です。

### **ビルド環境の構築**

1. **Visual Studioの設定**:  
   インストーラーで「C++ によるデスクトップ開発」を選択し、以下をインストールします。  
   * Windows 用 C++ CMake ツール  
   * Windows SDK  
   * x64/x86 用 MSVC ビルドツール  
2. **Ninjaのインストール**:  
   WinGet などで Ninja build をインストールします。  
3. **OpenCVの導入 (vcpkgを使用する場合)**:  
   * vcpkg を C:/Tools/ などにインストールします。  
   * ユーザー環境変数に VCPKG\_ROOT を追加し、PATH にも追記します。  
   * PowerShell から vcpkg install opencv:x64-windows-static でopencvをインストールします  
4. **プロジェクトの展開**:  
   * **x64 Native Tools Command Prompt for VS 2026** を開き、code と入力して VSCode を開きます。  
   * プロジェクトのフォルダを開きます。  
   * ※DLLを **x64** でビルドするために、毎回必ずこの手順（x64 Native Tools Command Prompt経由）で起動してください。  
5. **CMakeの設定**:  
   * CMakePresets.json の toolchainFile および CMAKE\_TOOLCHAIN\_FILE に、自身の環境の vcpkg.cmake のパスが正しく記述されているか確認してください。

### **ビルドの手順**

1. Ctrl \+ Shift \+ P \-\> CMake: Configure を実行し、release を選択します。  
2. Ctrl \+ Shift \+ P \-\> CMake: Build を実行します。  
3. プロジェクト直下の bin フォルダの中に Screencap.dll が生成されます。

## **ライセンス**

**MITライセンス**  
OpenCV以外はWindows標準のライブラリを使っています。詳細はLICENSE.txtを確認してください。