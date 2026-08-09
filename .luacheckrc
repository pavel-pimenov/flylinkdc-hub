std = "lua53"

-- Пропускать файлы из каталогов old/ и bug-lua/
exclude_files = {
    "scripts/old/",
    "scripts/bug-lua/",
}

-- PtokaX API globals
globals = {
    -- Top-level API tables
    "Core", "SetMan", "RegMan", "BanMan", "ProfMan",
    "TmrMan", "UDPDbg", "ScriptMan", "IP2Country",

    -- Callback functions defined by scripts
    "OnStartup", "OnExit", "OnError", "OnTimer",
    "ChatArrival", "KeyArrival", "ValidateNickArrival",
    "PasswordArrival", "VersionArrival", "GetNickListArrival",
    "MyINFOArrival", "GetINFOArrival", "SearchArrival",
    "ToArrival", "ConnectToMeArrival", "MultiConnectToMeArrival",
    "RevConnectToMeArrival", "SRArrival", "UDPSRArrival",
    "KickArrival", "OpForceMoveArrival", "SupportsArrival",
    "BotINFOArrival", "CloseArrival", "UnknownArrival",
    "ExtJSONArrival", "BadPassArrival", "ValidateDenideArrival",
    "UserConnected", "RegConnected", "OpConnected",
    "UserDisconnected", "RegDisconnected", "OpDisconnected",
}

-- Allow scripts to define their own globals (e.g. tCfg, bot, etc.)
allow_defined = true

read_globals = {
}

-- Ignore formatting warnings for now
-- 121: unused loop variable (often used for _)
-- 122: unused argument (callback signatures)
-- 231: inconsistent indentation (mixed tabs/spaces legacy)
-- 241: multiple consecutive blank lines
ignore = {"121", "122", "231", "241", "221"}

-- Max line length (script style varies, be lenient)
max_line_length = 160
