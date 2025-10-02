#Requires AutoHotkey v2
#SingleInstance

^0::
{
    startTime := A_TickCount
    RunWait A_ComSpec . ' /c python "' . A_WorkingDir . '\hello.py"'
    /*
    文字列の連結は '文字列１' . '文字列2' のように明示的にドットでつなぐか
    '文字列1' '文字列2' のようにスペースでつなぐ
    */
    ElapsedTime := A_TickCount - startTime
    ToolTip ElapsedTime . " milliseconds have elapsed."
    SetTimer () => ToolTip(), -5000
    /*
    SetTimer ToolTip, -5000でも動く
    ラムダ式を使っているのは
    1, 引数を渡しやすくするため
    SetTimer () => ToolTip("Hello"), -3000
    2, 複数処理をまとめることができる
    SetTimer () => (ToolTip("Hello"), Sleep(500), ToolTip("World")), -3000
    ラムダ式の書き方は
    () => (処理1, 処理2, ...)
    これが関数として認識される
    */
}
; 実行時間 750ミリ秒くらい。コマンドラインからpythonスクリプトを実行すると遅すぎる