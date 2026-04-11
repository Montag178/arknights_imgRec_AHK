#Requires AutoHotkey v2.0
#SingleInstance
SetWorkingDir "C:\Users\TumorNecrosisFactor\dev\montag\arknights_imgRec_AHK\build"
; DllCall("SetDllDirectory", "Str", "C:\Users\TumorNecrosisFactor\dev\montag\arknights_imgRec_AHK\build")
; #DllLoad "C:\Users\TumorNecrosisFactor\dev\montag\arknights_imgRec_AHK\build\Screencap.dll"
; #DllLoad "C:\Users\TumorNecrosisFactor\dev\montag\arknights_imgRec_AHK\build\Screencap.dll" ; keep the d3dDevice/d3dContext to prevent unloading
; DllCall("SetDllDirectory", "Str", A_ScriptDir "\build")
; hModule := DllCall("LoadLibrary", "Str", A_ScriptDir "\build\Screencap.dll", "Ptr")

^1::
{
    static rectBuffer := Buffer(8, 0)
    startTime  := A_TickCount
    try {
        result := DllCall("Screencap\RunProcess", 
        "Ptr", rectBuffer.Ptr, ; x cordinate
        "Ptr", rectBuffer.Ptr + 4, ; y cordinate
        "Int", 0, ; mode: 0 for skill, 1 for retreat
        "cdecl")
        ellapsedTime := A_TickCount - startTime
        if (result == 0) {
            ToolTip "Success: " . ellapsedTime . " ms" . Format(" (x: {}, y: {})", x, y)
            x := NumGet(rectBuffer, 0, "Int")
            y := NumGet(rectBuffer, 4, "Int")
        } else {
            MsgBox "Error code: " . Format("0x{:X}", result)
        }

    } catch as err {
        MsgBox Format("{1}: {2}.`n`nFile:`t{3}`nLine:`t{4}`nWhat:`t{5}`nStack:`n{6}"
        , type(err), err.Message, err.File, err.Line, err.What, err.Stack)
    }
    
    SetTimer () => ToolTip(), -5000
}
^2::
{
    static rectBuffer := Buffer(8, 0)
    startTime  := A_TickCount
    try {
        result := DllCall("Screencap\RunProcess", 
        "Ptr", rectBuffer.Ptr, ; x cordinate
        "Ptr", rectBuffer.Ptr + 4, ; y cordinate
        "Int", 1, ; mode: 0 for skill, 1 for retreat
        "cdecl")
        ellapsedTime := A_TickCount - startTime
        if (result == 0) {
            ToolTip "Success: " . ellapsedTime . " ms" . Format(" (x: {}, y: {})", x, y)
            x := NumGet(rectBuffer, 0, "Int")
            y := NumGet(rectBuffer, 4, "Int")
        } else {
            MsgBox "Error code: " . Format("0x{:X}", result)
        }

    } catch as err {
        MsgBox Format("{1}: {2}.`n`nFile:`t{3}`nLine:`t{4}`nWhat:`t{5}`nStack:`n{6}"
        , type(err), err.Message, err.File, err.Line, err.What, err.Stack)
    }
    
    SetTimer () => ToolTip(), -5000
}