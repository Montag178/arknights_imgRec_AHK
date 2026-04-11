#Requires AutoHotkey v2.0
#SingleInstance Force
DllCall("SetThreadDpiAwarenessContext", "ptr", -4, "ptr")
;=====================================================================================
; 以下を環境に合わせて調整してください
;=====================================================================================
; このファイルはbinと同じフォルダに配置するか、以下のようにSetWorkingDirでbinフォルダを指定してから実行してください
; SetWorkingDir "C:\your\path\to\bin"
SetWorkingDir A_ScriptDir . "\bin"
GroupAdd "Emulator", "ahk_exe dnplayer.exe" ; LDPlayer
; エミュレーターの画面解像度
res := [1600, 900]
; オペレーターを選択してからスクリーンキャプチャを始めるまでの時間
captureDelay := 150
; クリックが失敗したときにオペレーターの選択をキャンセルするまでの時間
cancelDelay := 50
; キャンセルするためにクリックする相対座標
cancelPos := [0.01, 0.1]


; ====================================================================================
; ライブラリ
; ====================================================================================
global g_sock := 0
global g_device := ""

InitFunc() 
OnExit(ExitFunc) 

InitFunc(){
    AdbInit()
}
ExitFunc(*) {
}

; ===== Winsock初期化・接続 =====
AdbConnect() {
    wsaData := Buffer(408, 0)
    if DllCall("ws2_32\WSAStartup", "UShort", 0x0202, "Ptr", wsaData) != 0
        throw Error("WSAStartup failed")

    sock := DllCall("ws2_32\socket", "Int", 2, "Int", 1, "Int", 6, "Ptr")
    if sock = -1
        throw Error("socket() failed")

    addr := Buffer(16, 0)
    NumPut("UShort", 2, addr, 0)
    NumPut("UShort", DllCall("ws2_32\htons", "UShort", 5037, "UShort"), addr, 2)
    NumPut("UInt", DllCall("ws2_32\inet_addr", "AStr", "127.0.0.1", "UInt"), addr, 4)

    if DllCall("ws2_32\connect", "Ptr", sock, "Ptr", addr, "Int", 16) != 0 {
        err := DllCall("ws2_32\WSAGetLastError")
        DllCall("ws2_32\closesocket", "Ptr", sock)
        throw Error("connect() failed: " err)
    }
    return sock
}

AdbSend(sock, cmd) {
    payload := Format("{:04X}", StrLen(cmd)) . cmd
    buf := Buffer(StrPut(payload, "CP0"), 0)
    StrPut(payload, buf, "CP0")
    DllCall("ws2_32\send", "Ptr", sock, "Ptr", buf, "Int", buf.Size - 1, "Int", 0)
}

AdbRecv(sock, bufSize := 4096) {
    buf := Buffer(bufSize, 0)
    bytes := DllCall("ws2_32\recv", "Ptr", sock, "Ptr", buf, "Int", bufSize, "Int", 0)
    return (bytes > 0) ? StrGet(buf, bytes, "CP0") : ""
}

AdbDisconnect(sock) {
    DllCall("ws2_32\closesocket", "Ptr", sock)
    DllCall("ws2_32\WSACleanup")
}

; ===== 初期化: デバイス名取得 =====
AdbInit() {
    sock := AdbConnect()
    AdbSend(sock, "host:devices")
    AdbRecv(sock)          ; OKAY
    raw := AdbRecv(sock)   ; 0015emulator-5554\tdevice\n

    ; デバイス名をパース（最初のタブまでの文字列）
    data := SubStr(raw, 5)   ; 先頭4文字(長さ)を除去
    g_device := Trim(SubStr(data, 1, InStr(data, "`t") - 1))

    AdbDisconnect(sock)

    if g_device = ""
        throw Error("デバイスが見つかりません. adb devicesでデバイスが接続されているか確認してください.")

    ; MsgBox "接続デバイス: " g_device
}

; ===== タップ送信 =====
AdbTap(x, y) {
    sock := AdbConnect()

    AdbSend(sock, "host:transport:" . g_device)
    if AdbRecv(sock) != "OKAY" {
        AdbDisconnect(sock)
        throw Error("transport failed")
    }

    AdbSend(sock, "shell:input tap " . x . " " . y)
    AdbRecv(sock)   ; OKAY

    ; 完了待ちを別スレッドに投げて即return
    sockCopy := sock
    SetTimer(() => _AdbWaitAndClose(sockCopy), -1)
}
_AdbWaitAndClose(sock) {
    ; 2個目のrecv(空の文字列). 受け取らないとタップ実行されない
    ; 100msぐらいかかるので,別スレッドで受け取る
    AdbRecv(sock)  
    AdbDisconnect(sock)
}

ADBClick(absolutePos) {
    WinGetClientPos ,, &W, &H, "A"
    x := Round(W / res[1] * absolutePos[1])
    y := Round(H / res[2] * absolutePos[2])
    AdbTap(x, y)
}

ADBRelativeClick(relativePos) {
    x := Round(res[1] * relativePos[1])
    y := Round(res[2] * relativePos[2])
    AdbTap(x, y)
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
            ADBClick([x1, y1])
            Sleep cancelDelay
            ADBRelativeClick(cancelPos)
        } else if (result == 1) {
            ADBRelativeClick(cancelPos)
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
            ADBClick([x1, y1])
            Sleep cancelDelay
            ADBRelativeClick(cancelPos)
        } else if (result == 1) {
            ADBRelativeClick(cancelPos)
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
        "cdecl")
        if (result == 0) {
            x1 := NumGet(coordsBuffer, 0, "Int")
            y1 := NumGet(coordsBuffer, 4, "Int")
            x2 := NumGet(coordsBuffer, 8, "Int")
            y2 := NumGet(coordsBuffer, 12, "Int")
            ADBClick([x2, y2])
        } else if (result == 1) {
            ADBRelativeClick(cancelPos) ; fallback to default position because no valid lines detected
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
RButton::skill()
; 撤退
XButton1::retreat()
; 選択キャンセル
; ^3::cancel()
XButton2::ADBRelativeClick(cancelPos)
#HotIf
