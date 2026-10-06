import os
import json
import ctypes
from ctypes import wintypes
from typing import Dict, Any, Optional

PIPE_NAME = r"\\.\pipe\ShaiyaOverlay_MCP"
BUFFER_SIZE = 65536
TIMEOUT_MS = 2500

kernel32 = ctypes.windll.kernel32
CallNamedPipeA = kernel32.CallNamedPipeA
CallNamedPipeA.argtypes = [
    wintypes.LPCSTR,
    wintypes.LPVOID,
    wintypes.DWORD,
    wintypes.LPVOID,
    wintypes.DWORD,
    ctypes.POINTER(wintypes.DWORD),
    wintypes.DWORD,
]
CallNamedPipeA.restype = wintypes.BOOL


def send_pipe_command(cmd_dict: Dict[str, Any], timeout_ms: int = TIMEOUT_MS) -> Optional[Dict[str, Any]]:
    """Sends JSON command to ShaiyaOverlay named pipe and returns parsed response."""
    req_json = json.dumps(cmd_dict).encode("utf-8")
    resp_buf = ctypes.create_string_buffer(BUFFER_SIZE)
    bytes_read = wintypes.DWORD(0)

    success = CallNamedPipeA(
        PIPE_NAME.encode("ascii"),
        req_json,
        len(req_json),
        resp_buf,
        BUFFER_SIZE - 1,
        ctypes.byref(bytes_read),
        timeout_ms,
    )

    if not success or bytes_read.value == 0:
        return None

    try:
        raw_text = resp_buf.value.decode("utf-8", errors="replace")
        return json.loads(raw_text)
    except Exception as e:
        return {"status": "error", "message": f"Failed to parse DLL response: {e}", "raw": resp_buf.value.decode("ascii", errors="ignore")}


def is_game_connected() -> bool:
    """Checks if the game DLL pipe is listening."""
    res = send_pipe_command({"cmd": "ping"}, timeout_ms=500)
    return res is not None and res.get("status") == "ok"


def query_status() -> Dict[str, Any]:
    """Queries overlay status, player vitals and navigation status."""
    res = send_pipe_command({"cmd": "get_status"})
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected. Named pipe inactive.",
        }
    res["game_connected"] = True
    return res


def query_player() -> Dict[str, Any]:
    """Queries comprehensive local player details (vitals, coordinates, direction, destination, state, camera)."""
    res = send_pipe_command({"cmd": "get_player"})
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def query_skills() -> Dict[str, Any]:
    """Queries list of all player skills with IDs, names, levels, cooldown remaining and duration."""
    res = send_pipe_command({"cmd": "get_skills"})
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def query_entities(type_filter: str = "all", max_distance: float = 300.0, limit: int = 50) -> Dict[str, Any]:
    """Queries nearby monsters, items on ground, and quest NPCs/markers."""
    res = send_pipe_command({
        "cmd": "get_entities",
        "type": type_filter,
        "max_distance": max_distance,
        "limit": limit,
    })
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def query_nearest_target(target_type: str = "any", max_distance: float = 300.0) -> Dict[str, Any]:
    """Finds closest entity to local player ('monster', 'item', 'npc', 'any')."""
    res = send_pipe_command({
        "cmd": "get_nearest_target",
        "type": target_type,
        "max_distance": max_distance,
    })
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def query_navigation() -> Dict[str, Any]:
    """Queries active auto-walk status, target position, remaining distance, and all calculated waypoints."""
    res = send_pipe_command({"cmd": "get_navigation"})
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def walk_to(x: float, y: Optional[float] = None, z: Optional[float] = None, name: str = "MCP_Target", stop_distance: float = 2.5) -> Dict[str, Any]:
    """Starts auto-walk navigation to 3D world coordinates with obstacle avoidance."""
    payload = {
        "cmd": "walk_to",
        "x": float(x),
        "z": float(z) if z is not None else 0.0,
        "name": name,
        "stop_distance": float(stop_distance),
    }
    if y is not None:
        payload["y"] = float(y)
    res = send_pipe_command(payload)
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def stop_walk() -> Dict[str, Any]:
    """Stops active auto-walking."""
    res = send_pipe_command({"cmd": "stop_walk"})
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def select_target(target_id: int) -> Dict[str, Any]:
    """Sets player's targeted entity in game memory."""
    res = send_pipe_command({"cmd": "select_target", "target_id": target_id})
    if res is None:
        return {"status": "offline", "game_connected": False}
    res["game_connected"] = True
    return res


def cast_skill(slot: int, target_id: Optional[int] = None, target_type: int = 3) -> Dict[str, Any]:
    """Casts a skill directly by its learned slot index."""
    payload: Dict[str, Any] = {"cmd": "cast_skill", "slot": slot, "target_type": target_type}
    if target_id is not None:
        payload["target_id"] = target_id
    res = send_pipe_command(payload)
    if res is None:
        return {"status": "offline", "game_connected": False}
    res["game_connected"] = True
    return res


def use_quickslot(slot: int) -> Dict[str, Any]:
    """Triggers an in-game quickslot (0-9) via native input event."""
    res = send_pipe_command({"cmd": "use_quickslot", "slot": slot})
    if res is None:
        return {"status": "offline", "game_connected": False}
    res["game_connected"] = True
    return res


def pickup_item(world_id: int) -> Dict[str, Any]:
    """Picks up a dropped ground item by its World ID using native SendPickUp."""
    res = send_pipe_command({"cmd": "pickup_item", "world_id": int(world_id)})
    if res is None:
        return {"status": "offline", "game_connected": False}
    res["game_connected"] = True
    return res


def set_autoloot(enabled: Optional[bool] = None, only_my_drops: Optional[bool] = None,
                 auto_walk: Optional[bool] = None, radius: Optional[float] = None) -> Dict[str, Any]:
    """Configures the automatic loot pickup system."""
    payload: Dict[str, Any] = {"cmd": "set_autoloot"}
    if enabled is not None:
        payload["enabled"] = bool(enabled)
    if only_my_drops is not None:
        payload["only_my_drops"] = bool(only_my_drops)
    if auto_walk is not None:
        payload["auto_walk"] = bool(auto_walk)
    if radius is not None:
        payload["radius"] = float(radius)
    res = send_pipe_command(payload)
    if res is None:
        return {"status": "offline", "game_connected": False}
    res["game_connected"] = True
    return res


def set_autobuff(enabled: Optional[bool] = None, skill_id: Optional[int] = None,
                 active: Optional[bool] = None, recast_seconds: Optional[int] = None) -> Dict[str, Any]:
    """Configures automatic buff recast for specified skills."""
    payload: Dict[str, Any] = {"cmd": "set_autobuff"}
    if enabled is not None:
        payload["enabled"] = bool(enabled)
    if skill_id is not None:
        payload["skill_id"] = int(skill_id)
    if active is not None:
        payload["active"] = bool(active)
    if recast_seconds is not None:
        payload["recast_seconds"] = int(recast_seconds)
    res = send_pipe_command(payload)
    if res is None:
        return {"status": "offline", "game_connected": False}
    res["game_connected"] = True
    return res


def query_combo() -> Dict[str, Any]:
    """Retrieves auto-combo configuration, active state, and skill sequence."""
    res = send_pipe_command({"cmd": "get_combo"})
    if res is None:
        return {"status": "offline", "game_connected": False}
    res["game_connected"] = True
    return res


def set_autocombo(enabled: Optional[bool] = None, active: Optional[bool] = None,
                  hold_mode: Optional[bool] = None, delay_ms: Optional[int] = None,
                  auto_target: Optional[bool] = None, target_filter: Optional[int] = None,
                  max_target_range: Optional[float] = None,
                  add_skill_id: Optional[int] = None, clear: Optional[bool] = None) -> Dict[str, Any]:
    """Configures the auto-combo rotation sequence, timing, target acquisition, and execution state."""
    payload: Dict[str, Any] = {"cmd": "set_autocombo"}
    if enabled is not None:
        payload["enabled"] = bool(enabled)
    if active is not None:
        payload["active"] = bool(active)
    if hold_mode is not None:
        payload["hold_mode"] = bool(hold_mode)
    if delay_ms is not None:
        payload["delay_ms"] = int(delay_ms)
    if auto_target is not None:
        payload["auto_target"] = bool(auto_target)
    if target_filter is not None:
        payload["target_filter"] = int(target_filter)
    if max_target_range is not None:
        payload["max_target_range"] = float(max_target_range)
    if add_skill_id is not None:
        payload["add_skill_id"] = int(add_skill_id)
    if clear is not None:
        payload["clear"] = bool(clear)
    res = send_pipe_command(payload)
    if res is None:
        return {"status": "offline", "game_connected": False}
    res["game_connected"] = True
    return res


def query_inventory() -> Dict[str, Any]:
    """Retrieves all player inventory items with bag, slot, count, name, and consumable status."""
    res = send_pipe_command({"cmd": "get_inventory"})
    if res is None:
        return {"status": "offline", "game_connected": False, "item_count": 0, "items": []}
    res["game_connected"] = True
    return res


def query_buffs() -> Dict[str, Any]:
    """Retrieves all active player buffs and debuffs with level, duration remaining, and name."""
    res = send_pipe_command({"cmd": "get_buffs"})
    if res is None:
        return {"status": "offline", "game_connected": False, "buff_count": 0, "buffs": []}
    res["game_connected"] = True
    return res


def auto_login(username: Optional[str] = None, password: Optional[str] = None) -> Dict[str, Any]:
    """Triggers or checks the automatic login state machine."""
    cmd_data = {"cmd": "auto_login"}
    if username:
        cmd_data["username"] = username
    if password:
        cmd_data["password"] = password
    res = send_pipe_command(cmd_data)
    if res is None:
        return {"status": "offline", "game_connected": False}
    res["game_connected"] = True
    return res


def walk_to_target(target_type: str = "any", name_filter: Optional[str] = None, stop_distance: float = 2.5) -> Dict[str, Any]:
    """Finds nearest NPC, monster, quest mob or item (optionally matching name_filter) and starts pathfinding walk_to."""
    is_quest_filter = (target_type in ["quest", "quest_mob"])
    effective_type = "monster" if is_quest_filter else target_type
    ents = query_entities(type_filter="all" if effective_type == "any" else effective_type, max_distance=500.0, limit=100)
    if not ents.get("entities"):
        return {"status": "error", "message": f"No {target_type} found in range"}

    best_ent = None
    for e in ents["entities"]:
        if is_quest_filter:
            if not e.get("is_quest"):
                continue
        elif target_type != "any" and e.get("type") != target_type:
            continue
        if name_filter:
            if name_filter.lower() not in e.get("name", "").lower():
                continue
        if best_ent is None or e.get("distance", 9999) < best_ent.get("distance", 9999):
            best_ent = e

    if not best_ent:
        return {"status": "error", "message": f"No matching {target_type} found for query '{name_filter}'"}

    pos = best_ent["pos"]
    name = best_ent.get("name", "Target")
    return walk_to(x=pos[0], y=pos[1], z=pos[2], name=name, stop_distance=stop_distance)


def check_collision(start_x: float, start_y: float, start_z: float, end_x: float, end_y: float, end_z: float, radius: float = 0.75) -> Dict[str, Any]:
    """Tests collision between two 3D world points (line-of-sight, clearance corridor, slope)."""
    res = send_pipe_command({
        "cmd": "check_collision",
        "start_x": float(start_x),
        "start_y": float(start_y),
        "start_z": float(start_z),
        "end_x": float(end_x),
        "end_y": float(end_y),
        "end_z": float(end_z),
        "radius": float(radius),
    })
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def get_ground_height(x: float, z: float) -> Dict[str, Any]:
    """Queries ground surface elevation Y at world coordinate (X, Z)."""
    res = send_pipe_command({
        "cmd": "get_ground_height",
        "x": float(x),
        "z": float(z),
    })
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def diagnose_path(goal_x: float, goal_z: float, goal_y: Optional[float] = None,
                  start_x: Optional[float] = None, start_y: Optional[float] = None, start_z: Optional[float] = None) -> Dict[str, Any]:
    """Computes and inspects full pathfinder waypoints from Start (or Player Pos) to Goal without moving."""
    payload = {
        "cmd": "diagnose_path",
        "goal_x": float(goal_x),
        "goal_z": float(goal_z),
    }
    if goal_y is not None:
        payload["goal_y"] = float(goal_y)
    if start_x is not None and start_z is not None:
        payload["start_x"] = float(start_x)
        payload["start_z"] = float(start_z)
        if start_y is not None:
            payload["start_y"] = float(start_y)

    res = send_pipe_command(payload)
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def read_memory(address: Any, size: int = 64) -> Dict[str, Any]:
    """Safely reads a memory block from game process."""
    addr_str = hex(address) if isinstance(address, int) else str(address)
    res = send_pipe_command({
        "cmd": "read_memory",
        "address": addr_str,
        "size": size,
    })
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def write_memory(address: Any, hex_data: str) -> Dict[str, Any]:
    """Safely writes hex bytes to game process memory."""
    addr_str = hex(address) if isinstance(address, int) else str(address)
    res = send_pipe_command({
        "cmd": "write_memory",
        "address": addr_str,
        "hex": hex_data,
    })
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def world_to_screen(x: float, y: float, z: float) -> Dict[str, Any]:
    """Converts 3D world coordinates to 2D screen coordinates."""
    res = send_pipe_command({
        "cmd": "world_to_screen",
        "x": float(x),
        "y": float(y),
        "z": float(z),
    })
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def toggle_menu(open_val: Optional[bool] = None) -> Dict[str, Any]:
    """Toggles or sets overlay menu visibility."""
    payload = {"cmd": "toggle_menu"}
    if open_val is not None:
        payload["open"] = bool(open_val)
    res = send_pipe_command(payload)
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def query_config() -> Dict[str, Any]:
    """Queries overlay settings in memory."""
    res = send_pipe_command({"cmd": "get_config"})
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def set_config(key: str, value: bool) -> Dict[str, Any]:
    """Updates overlay settings (e.g. 'snaplines_enabled', 'quest_waypoints_enabled', 'menu_open')."""
    res = send_pipe_command({
        "cmd": "set_config",
        "key": key,
        "value": value,
    })
    if res is None:
        return {
            "status": "offline",
            "game_connected": False,
            "message": "Game is not running or ShaiyaOverlay.dll is not injected.",
        }
    res["game_connected"] = True
    return res


def unload_dll() -> Dict[str, Any]:
    """Unloads and ejects ShaiyaOverlay.dll from the game process."""
    res = send_pipe_command({"cmd": "unload"})
    if res is not None:
        return res

    # Fallback via PostMessage VK_END (0x23) to game window
    try:
        user32 = ctypes.windll.user32
        hwnd = user32.FindWindowA(b"SDL_app", None)
        if hwnd:
            user32.PostMessageA(hwnd, 0x0101, 0x23, 0)
            return {"status": "ok", "action": "unload", "method": "fallback_postmessage_vk_end"}
    except Exception as e:
        return {"status": "error", "message": f"Unload failed: {e}"}

    return {"status": "error", "message": "Overlay pipe not responding and game window not found"}


TH32CS_SNAPPROCESS = 0x00000002

class PROCESSENTRY32(ctypes.Structure):
    _fields_ = [
        ('dwSize', wintypes.DWORD),
        ('cntUsage', wintypes.DWORD),
        ('th32ProcessID', wintypes.DWORD),
        ('th32DefaultHeapID', ctypes.POINTER(ctypes.c_ulong)),
        ('th32ModuleID', wintypes.DWORD),
        ('cntThreads', wintypes.DWORD),
        ('th32ParentProcessID', wintypes.DWORD),
        ('pcPriClassBase', ctypes.c_long),
        ('dwFlags', wintypes.DWORD),
        ('szExeFile', ctypes.c_char * 260)
    ]

def find_game_pid(exe_name: str = "game.exe") -> Optional[int]:
    """Finds process ID of game.exe using Toolhelp API."""
    h = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    if h == wintypes.HANDLE(-1).value or h == -1:
        return None
    pe = PROCESSENTRY32()
    pe.dwSize = ctypes.sizeof(PROCESSENTRY32)
    if kernel32.Process32First(h, ctypes.byref(pe)):
        while True:
            cur_exe = pe.szExeFile.decode('latin1', errors='ignore').lower()
            if cur_exe == exe_name.lower():
                kernel32.CloseHandle(h)
                return pe.th32ProcessID
            if not kernel32.Process32Next(h, ctypes.byref(pe)):
                break
    kernel32.CloseHandle(h)
    return None


kernel32.OpenProcess.restype = wintypes.HANDLE
kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]

kernel32.VirtualAllocEx.restype = ctypes.c_void_p
kernel32.VirtualAllocEx.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]

kernel32.WriteProcessMemory.restype = wintypes.BOOL
kernel32.WriteProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]

kernel32.GetModuleHandleA.restype = wintypes.HMODULE
kernel32.GetModuleHandleA.argtypes = [wintypes.LPCSTR]

kernel32.GetProcAddress.restype = ctypes.c_void_p
kernel32.GetProcAddress.argtypes = [wintypes.HMODULE, wintypes.LPCSTR]

kernel32.CreateRemoteThread.restype = wintypes.HANDLE
kernel32.CreateRemoteThread.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)]


def load_dll(dll_path: Optional[str] = None) -> Dict[str, Any]:
    """Injects / loads ShaiyaOverlay.dll into game.exe process."""
    if dll_path is None:
        dll_path = r"G:\Games\Shaiya\default\game\ShaiyaOverlay\Bin\Release\ShaiyaOverlay.dll"

    dll_path = os.path.abspath(dll_path)
    if not os.path.exists(dll_path):
        return {"status": "error", "message": f"DLL not found at: {dll_path}"}

    pid = find_game_pid("game.exe")
    if not pid:
        return {"status": "error", "message": "game.exe process not running"}

    PROCESS_ALL_ACCESS = 0x1F0FFF
    MEM_COMMIT_RESERVE = 0x3000
    PAGE_READWRITE = 0x04

    h_process = kernel32.OpenProcess(PROCESS_ALL_ACCESS, False, pid)
    if not h_process:
        err = kernel32.GetLastError()
        return {"status": "error", "message": f"OpenProcess failed on PID {pid}, error: {err}"}

    dll_bytes = dll_path.encode("ascii") + b"\0"
    remote_mem = kernel32.VirtualAllocEx(h_process, None, len(dll_bytes), MEM_COMMIT_RESERVE, PAGE_READWRITE)
    if not remote_mem:
        err = kernel32.GetLastError()
        kernel32.CloseHandle(h_process)
        return {"status": "error", "message": f"VirtualAllocEx failed, error: {err}"}

    bytes_written = ctypes.c_size_t(0)
    kernel32.WriteProcessMemory(h_process, remote_mem, dll_bytes, len(dll_bytes), ctypes.byref(bytes_written))

    h_k32 = kernel32.GetModuleHandleA(b"kernel32.dll")
    p_load_lib = kernel32.GetProcAddress(h_k32, b"LoadLibraryA")
    thread_id = wintypes.DWORD(0)
    h_thread = kernel32.CreateRemoteThread(h_process, None, 0, p_load_lib, remote_mem, 0, ctypes.byref(thread_id))

    if not h_thread:
        err = kernel32.GetLastError()
        kernel32.CloseHandle(h_process)
        return {"status": "error", "message": f"CreateRemoteThread failed, error: {err}"}

    kernel32.WaitForSingleObject(h_thread, 3000)
    kernel32.CloseHandle(h_thread)
    kernel32.CloseHandle(h_process)

    return {"status": "ok", "action": "load_dll", "pid": pid, "dll_path": dll_path}
