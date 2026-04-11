#Requires AutoHotkey v2.0
#SingleInstance

; このファイルはdllと同じフォルダに配置するか、SetWorkingDirでdllのあるフォルダを指定してから実行してください
SetWorkingDir "C:\Users\TumorNecrosisFactor\dev\montag\arknights_imgRec_AHK\build"

; スクリーンキャプチャーのテスト, 成功するとキャプチャー画像が表示されます
; 表示されたウィンドウは何らかのキーを押して閉じることができます
^1::
{
    try {
        result := DllCall("Screencap\CaptureTest", "cdecl int")
        if (result != 0) {
            MsgBox "Error code: " . Format("0x{:X}", result)
        }
    } catch as err {
        MsgBox Format("{1}: {2}.`n`nFile:`t{3}`nLine:`t{4}`nWhat:`t{5}`nStack:`n{6}"
        , type(err), err.Message, err.File, err.Line, err.What, err.Stack)
    }
}
; ライン検出のテスト, オペレータを選択してひし形を表示してから実行してください
^2::
{
    try {
        result := DllCall("Screencap\ShowProcessedImage", "cdecl int")
        if (result != 0) {
            if (result == 1) {
                MsgBox "No valid lines detected. Try adjusting the threshold or check if the game window is properly positioned."
            } else {
                MsgBox "Error code: " . Format("0x{:X}", result)
            }
        } else {
            ; Read and display debug info
            if FileExist("debug_info.txt") {
                debugContent := FileRead("debug_info.txt")
                MsgBox "Debug Info:`n" . debugContent
            }
        }
    } catch as err {
        MsgBox Format("{1}: {2}.`n`nFile:`t{3}`nLine:`t{4}`nWhat:`t{5}`nStack:`n{6}"
        , type(err), err.Message, err.File, err.Line, err.What, err.Stack)
    }
}
; 実行時間測定, オペレータを選択してひし形を表示してから実行してください
; 初回はdllのロードに少し時間がかかります
^3::
{
    static coordsBuffer := Buffer(16, 0)
    startTime  := A_TickCount
    try {
        result := DllCall("Screencap\RunProcess", 
        "Ptr", coordsBuffer.Ptr, ; x cordinate
        "Ptr", coordsBuffer.Ptr + 4, ; y cordinate
        "Ptr", coordsBuffer.Ptr + 8, ; x cordinate for cancel
        "Ptr", coordsBuffer.Ptr + 12, ; y cordinate for cancel
        "Int", 0, ; mode: 0 for skill, 1 for retreat
        "cdecl int")
        ellapsedTime := A_TickCount - startTime
        if (result == 0) {
            x1 := NumGet(coordsBuffer, 0, "Int")
            y1 := NumGet(coordsBuffer, 4, "Int")
            x2 := NumGet(coordsBuffer, 8, "Int")
            y2 := NumGet(coordsBuffer, 12, "Int")
            ToolTip "Success: " . ellapsedTime . " ms" . Format(" (x: {}, y: {})", x1, y1)
        } else if (result == 1) {
            ToolTip "No valid lines detected. Try adjusting the threshold or check if the game window is properly positioned."
        } else {
            MsgBox "Error code: " . Format("0x{:X}", result)
        }

    } catch as err {
        MsgBox Format("{1}: {2}.`n`nFile:`t{3}`nLine:`t{4}`nWhat:`t{5}`nStack:`n{6}"
        , type(err), err.Message, err.File, err.Line, err.What, err.Stack)
    }
    SetTimer () => ToolTip(), -5000
}