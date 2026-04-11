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
  * LDPlayerなどのエミュレーターではControlClickが不安定な場合があります。
  * その場合は arknights\_imgRec\_adb.ahk を試してみてください。
    * ※エミュレーターを起動してadb connectしてからスクリプトを起動してください

## **使い方**

1. **ダウンロード**: \[Releases\] からZIPファイルをダウンロードして展開します。  
2. **設定**:  
   * 使うエミュレータのファイル名（.exe）をスクリプトの中で指定してください。  
   * ahkスクリプトを bin フォルダと違う場所に置く場合は、スクリプトの中の SetWorkingDir でパスを指定する必要があります。  
3. **起動**: ahkファイルを実行します。  
   * ※**AutoHotkey v2 (64-bit)** での起動が必須です。32-bit版では動きません。

デフォルトのキーバインドは以下です

| キー操作 | 動作 |
| :---- | :---- |
| **Ctrl + 1** | スキル発動 |
| **Ctrl + 2** | 撤退 |
| **Ctrl + 3** | 選択キャンセル |

## **ビルド方法**

### **必要なもの**

* Visual Studio 2022 (Build Tools または Community)  
* Ninja build  
* VSCode (拡張機能: C/C++, CMake Tools, AHK++)  
* **OpenCV**: vcpkg を使用するか、[公式サイト](https://www.google.com/search?q=https://opencv.org/releases/)から取得してください。  
* **ヘッダーファイル**: direct3d11.interop.h が必要です。

### **フォルダ構成（ビルド時）**

プロジェクトのルートディレクトリに以下のフォルダを作成して配置してください。

* third\_party/direct3d11.interop.h がプロジェクトフォルダに配置されている必要があります。
* OpenCVを自分で用意する場合、CMakeが検出できるパスに配置してください（vcpkg推奨）。

### **ビルド環境の構築**

1. Visual Studio インストーラーで「C++ によるデスクトップ開発」を選択し、以下が含まれていることを確認します。  
   * Windows 用 C++ CMake ツール  
   * Windows SDK  
   * x64/x86 用 MSVC ビルドツール  
2. **x64 Native Tools Command Prompt for VS 2026** を開きます。  
3. コマンドプロンプトから code . と入力してプロジェクトを開きます。  
   * DLLを **x64** でビルドするために必ずこの手順を踏んでください。  
vcpkgの導入の参考はこちら  
https://learn.microsoft.com/ja-jp/vcpkg/get_started/get-started-vscode?pivots=shell-powershell

### **ビルドの手順**

1. VSCodeでプロジェクトフォルダを開きます。  
2. Ctrl \+ Shift \+ P で CMake: Configure を実行し、キットに ninja-msvc 等を選択します。  
3. Ctrl \+ Shift \+ P で CMake: Build を実行します。  
4. build フォルダの中に Screencap.dll が生成されます。

## **ライセンス**

**MITライセンス**  
OpenCV以外はWindows標準のライブラリを使っています。自由に改変および配布していたいただけます。詳細はLICENSE.txtを確認してください。
