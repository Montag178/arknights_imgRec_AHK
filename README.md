ビルドに必要なもの
Build Tools for Visual Studio 2022 もしくは Visual Studio Community 2022
Ninja build

## ビルド環境の用意（VScode + Cmake）
- Visual Studio Build Tools（最新版でOK）をダウンロード
  - インストール画面でチェックを入れる必要があるもの（おそらく）
    - Windows用 C++ CMakeツール
    - Windows SDK (10でも11でも)
    - x64/x86用MSVCビルドツール（最新）
- Ninja Build (WinGetで入手できる)
- x64 Native Tools Command Prompt for VSからcodeコマンドでVSCodeを開く
  - こうしないとコンパイラにMSVCを指定できない[参考](https://learn.microsoft.com/ja-jp/vcpkg/get_started/get-started-vscode?pivots=shell-powershell)
  - ahkから呼び出すにはx64でビルドする必要があるのでx64 Native Tools Command Prompt for VSから起動すると確実
  - このプロジェクトを開くときは毎回この手順で開く必要がある。（そうしなくていい設定もあるらしい）
- VSCodeの拡張機能を入れる
  - C/C++ (DevTools, Extention Pack, Themes)
  - CMake Tools
  - AHK++ （AHKのコードを編集するときにあると便利）
- たぶんほかに設定は必要なかったはず。Shift+Ctrl+PのコマンドパレットからCMake:Buildを選んでビルド
- buildフォルダにビルドしたものが生成される
