#Requires AutoHotkey v2.0
#SingleInstance

^0::
{
    startTime  := A_TickCount
    hPipe := DllCall("CreateFile"
        , "Str", "\\.\pipe\MyPipe"
        , "UInt", 0x40000000  ; GENERIC_WRITE
        , "UInt", 0
        , "UInt", 0
        , "UInt", 3           ; OPEN_EXISTING
        , "UInt", 0
        , "UInt", 0
        , "Ptr")
    if (hPipe = -1) {
        MsgBox "Failed to connect to pipe"
        return
    }
    msg := "ping"
    DllCall("WriteFile"
        , "Ptr", hPipe
        , "Str", msg
        , "UInt", StrLen(msg)
        , "UIntP", 0
        , "Ptr", 0)
    DllCall("CloseHandle", "Ptr", hPipe)
    ellapsedTime := A_TickCount - startTime
    ToolTip ellapsedTime . " milliseconds have elapsed."
    SetTimer () => ToolTip(), -5000
}