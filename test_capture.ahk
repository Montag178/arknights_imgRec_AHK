#Requires AutoHotkey v2.0
#SingleInstance
SetWorkingDir "C:\Users\TumorNecrosisFactor\dev\montag\arknights_imgRec_AHK\build"

^0::
{
    startTime  := A_TickCount
    ; DllCall("SetDllDirectory", "Str", "C:\Windows\System32")
    ; h := DllCall("LoadLibrary", "Str", "Screencap.dll", "Ptr")
    ; MsgBox A_LastError
    ; DllCall("Screencap.dll\RunProcess", "cdecl")
    ; DllCall("FreeLibrary", "Ptr", h)
    ; hModule := DllCall("LoadLibrary", "Str", "C:\Users\TumorNecrosisFactor\dev\montag\arknights_imgRec_AHK\build\Screencap.dll", "Ptr")
    ; MsgBox A_LastError
    ; DllCall("FreeLibrary", "Ptr", hModule)
    ; MsgBox A_LastError
    try {
        result := DllCall("Screencap\RunProcess", "cdecl")
        ellapsedTime := A_TickCount - startTime
        if (result == 0) {
            ToolTip "Success: " . ellapsedTime . " ms"
        } else {
            MsgBox "Error code: " . Format("0x{:X}", result)
        }
    } catch as err {
        MsgBox Format("{1}: {2}.`n`nFile:`t{3}`nLine:`t{4}`nWhat:`t{5}`nStack:`n{6}"
        , type(err), err.Message, err.File, err.Line, err.What, err.Stack)
    }
    
    SetTimer () => ToolTip(), -5000
}