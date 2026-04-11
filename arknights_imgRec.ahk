#Requires AutoHotkey v2.0
#SingleInstance Force
DllCall("SetThreadDpiAwarenessContext", "ptr", -4, "ptr")
;=====================================================================================
; 以下を環境に合わせて調整してください
;=====================================================================================
; このファイルはbinと同じフォルダに配置するか、以下のようにSetWorkingDirでbinフォルダを指定してから実行してください
; SetWorkingDir "C:\your\path\to\bin"
SetWorkingDir A_ScriptDir . "\bin"
GroupAdd "Emulator", "ahk_exe crosvm.exe" ; GooglePlayGames
; オペレーターを選択してからスクリーンキャプチャを始めるまでの時間
captureDelay := 150
; クリックが失敗したときにオペレーターの選択をキャンセルするまでの時間
cancelDelay := 50
; キャンセルするためにクリックする相対座標
cancelPos := [0.01, 0.1]

; ====================================================================================
; ライブラリ
; ====================================================================================
; [横位置の比率, 縦位置の比率]で指定した位置をクリックする
RelativeClick(relativePos) {
	WinGetClientPos ,, &W, &H, "A"
	x := Round(W * relativePos[1])
	y := Round(H * relativePos[2])
	ControlClick Format("x{} y{}", x, y), "A"
}

;=====================================================================================
; ホットキー関数
;=====================================================================================
skill()
{
    Send "{LButton}" 
    Sleep captureDelay
    static coordsBuffer := Buffer(16, 0)
    try {
        result := DllCall("Screencap\RunProcess", 
        "Ptr", coordsBuffer.Ptr, ; x cordinate
        "Ptr", coordsBuffer.Ptr + 4, ; y cordinate
        "Ptr", coordsBuffer.Ptr + 8, ; x cordinate for cancel
        "Ptr", coordsBuffer.Ptr + 12, ; y cordinate for cancel
        "Int", 0, ; mode: 0 for skill, 1 for retreat
        "cdecl int")
        if (result == 0) {
            x1 := NumGet(coordsBuffer, 0, "Int")
            y1 := NumGet(coordsBuffer, 4, "Int")
            x2 := NumGet(coordsBuffer, 8, "Int")
            y2 := NumGet(coordsBuffer, 12, "Int")
            ControlClick Format("x{} y{}", x1, y1), "A"
            Sleep cancelDelay
            RelativeClick(cancelPos) ; fallback to default position if the first click failed
            ; ControlClick Format("x{} y{}", x2, y2), "A"
        } else if (result == 1) {
            RelativeClick(cancelPos) ; fallback to default position because no valid lines detected
        } else {
            MsgBox "Error code: " . Format("0x{:X}", result)
        }
    } catch as err {
        MsgBox Format("{1}: {2}.`n`nFile:`t{3}`nLine:`t{4}`nWhat:`t{5}`nStack:`n{6}"
        , type(err), err.Message, err.File, err.Line, err.What, err.Stack)
    }
}

retreat()
{
    Send "{LButton}" 
    Sleep captureDelay
    static coordsBuffer := Buffer(16, 0)
    try {
        result := DllCall("Screencap\RunProcess", 
        "Ptr", coordsBuffer.Ptr, ; x cordinate
        "Ptr", coordsBuffer.Ptr + 4, ; y cordinate
        "Ptr", coordsBuffer.Ptr + 8, ; x cordinate for cancel
        "Ptr", coordsBuffer.Ptr + 12, ; y cordinate for cancel
        "Int", 1, ; mode: 0 for skill, 1 for retreat
        "cdecl int")
        if (result == 0) {
            x1 := NumGet(coordsBuffer, 0, "Int")
            y1 := NumGet(coordsBuffer, 4, "Int")
            x2 := NumGet(coordsBuffer, 8, "Int")
            y2 := NumGet(coordsBuffer, 12, "Int")
            ControlClick Format("x{} y{}", x1, y1), "A"
            Sleep cancelDelay
            RelativeClick(cancelPos) ; fallback to default position if the first click failed
        } else if (result == 1) {
            RelativeClick(cancelPos) ; fallback to default position because no valid lines detected
        } else {
            MsgBox "Error code: " . Format("0x{:X}", result)
        }
    } catch as err {
        MsgBox Format("{1}: {2}.`n`nFile:`t{3}`nLine:`t{4}`nWhat:`t{5}`nStack:`n{6}"
        , type(err), err.Message, err.File, err.Line, err.What, err.Stack)
    }
}

cancel()
{
    static coordsBuffer := Buffer(16, 0)
    try {
        result := DllCall("Screencap\RunProcess", 
        "Ptr", coordsBuffer.Ptr, ; x cordinate
        "Ptr", coordsBuffer.Ptr + 4, ; y cordinate
        "Ptr", coordsBuffer.Ptr + 8, ; x cordinate for cancel
        "Ptr", coordsBuffer.Ptr + 12, ; y cordinate for cancel
        "Int", 1, ; mode: 0 for skill, 1 for retreat
        "cdecl int")
        if (result == 0) {
            x1 := NumGet(coordsBuffer, 0, "Int")
            y1 := NumGet(coordsBuffer, 4, "Int")
            x2 := NumGet(coordsBuffer, 8, "Int")
            y2 := NumGet(coordsBuffer, 12, "Int")
            ControlClick Format("x{} y{}", x2, y2), "A" ; cancel if the first click failed
        } else if (result == 1) {
            RelativeClick(cancelPos) ; fallback to default position because no valid lines detected
        } else {
            MsgBox "Error code: " . Format("0x{:X}", result)
        }
    } catch as err {
        MsgBox Format("{1}: {2}.`n`nFile:`t{3}`nLine:`t{4}`nWhat:`t{5}`nStack:`n{6}"
        , type(err), err.Message, err.File, err.Line, err.What, err.Stack)
    }
}

; ====================================================================================
; ホットキー定義
; ====================================================================================
; エミュレーターがアクティブな場合
#HotIf WinActive("ahk_group Emulator")
; スキル使用
^1::skill()
; 撤退
^2::retreat()
; 選択キャンセル
; ^3::cancel()
^3::RelativeClick(cancelPos)
#HotIf
