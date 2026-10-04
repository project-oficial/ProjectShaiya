import os
import sys
from typing import Any, Optional, Dict
from mcp.server.fastmcp import FastMCP

# Ensure directory is on python path
current_dir = os.path.dirname(os.path.abspath(__file__))
if current_dir not in sys.path:
    sys.path.insert(0, current_dir)

import mcp_client

mcp = FastMCP("ShaiyaOverlay-MCPTool")


@mcp.tool()
def shaiya_get_status() -> dict:
    """Get current Shaiya game and overlay status: PID, player HP/MaxHP/Level/Coords, active navigation state, and menu visibility."""
    return mcp_client.query_status()


@mcp.tool()
def shaiya_get_player() -> dict:
    """Get comprehensive details about local player: HP, MaxHP, Level, Position (X, Y, Z), Direction (X, Y, Z), Destination, State flags, and Camera position."""
    return mcp_client.query_player()


@mcp.tool()
def shaiya_get_skills() -> dict:
    """Get list of all player skills with IDs, names, levels, learned status, ready status, cooldown remaining, and total cooldown duration."""
    return mcp_client.query_skills()


@mcp.tool()
def shaiya_get_entities(type_filter: str = "all", max_distance: float = 300.0, limit: int = 50) -> dict:
    """Query nearby entities loaded in memory.
    Parameters:
      type_filter: 'all', 'monster', 'item', or 'npc'
      max_distance: Search radius in game units (default 300.0)
      limit: Maximum number of entities to return (default 50)
    """
    return mcp_client.query_entities(type_filter=type_filter, max_distance=max_distance, limit=limit)


@mcp.tool()
def shaiya_get_nearest_target(target_type: str = "any", max_distance: float = 300.0) -> dict:
    """Find the single closest matching entity to local player.
    Parameters:
      target_type: 'any', 'monster', 'item', 'npc'
      max_distance: Search radius in game units (default 300.0)
    """
    return mcp_client.query_nearest_target(target_type=target_type, max_distance=max_distance)


@mcp.tool()
def shaiya_get_navigation() -> dict:
    """Get complete real-time navigation status: target name, target coordinates, remaining distance, waypoint count, current waypoint index, and full array of waypoints."""
    return mcp_client.query_navigation()


@mcp.tool()
def shaiya_walk_to(x: float, z: float, y: Optional[float] = None, name: str = "MCP_Target", stop_distance: float = 2.5) -> dict:
    """Start auto-walking to 3D world coordinates using the native obstacle-avoiding A* pathfinder.
    Parameters:
      x: Target X coordinate
      z: Target Z coordinate
      y: Target Y coordinate (optional; if omitted, automatically queries ground height)
      name: Destination label for display/logs
      stop_distance: Arrival distance in meters (default 2.5)
    """
    return mcp_client.walk_to(x=x, y=y, z=z, name=name, stop_distance=stop_distance)


@mcp.tool()
def shaiya_walk_to_target(target_type: str = "any", name_filter: Optional[str] = None, stop_distance: float = 2.5) -> dict:
    """Finds nearest NPC, monster, or ground item (optionally matching name_filter) and starts A* pathfinder auto-walk to it.
    Parameters:
      target_type: 'npc', 'monster', 'item', or 'any'
      name_filter: Substring of entity name to search for (e.g. 'Javali', 'Quest', 'Larva')
      stop_distance: Arrival distance in meters (default 2.5)
    """
    return mcp_client.walk_to_target(target_type=target_type, name_filter=name_filter, stop_distance=stop_distance)


@mcp.tool()
def shaiya_stop_walk() -> dict:
    """Stop auto-walk navigation immediately."""
    return mcp_client.stop_walk()


@mcp.tool()
def shaiya_select_target(target_id: int) -> dict:
    """Select a specific target entity by WorldId."""
    return mcp_client.select_target(target_id=target_id)


@mcp.tool()
def shaiya_cast_skill(slot: int, target_id: Optional[int] = None, target_type: int = 3) -> dict:
    """Cast a skill directly by slot index (0 = Interpretação, 1 = Tranquilidade, 2 = Flecha Mágica)."""
    return mcp_client.cast_skill(slot=slot, target_id=target_id, target_type=target_type)


@mcp.tool()
def shaiya_use_quickslot(slot: int) -> dict:
    """Trigger an in-game quickslot key (0-9 for bar 1 keys 1-0)."""
    return mcp_client.use_quickslot(slot=slot)


@mcp.tool()
def shaiya_get_inventory() -> dict:
    """List player inventory items with bag, slot, count, item name and consumable status."""
    return mcp_client.query_inventory()


@mcp.tool()
def shaiya_get_buffs() -> dict:
    """List all active player buffs and debuffs with level, remaining duration and effect name."""
    return mcp_client.query_buffs()


@mcp.tool()
def shaiya_auto_login(username: str = "", password: str = "") -> dict:
    """Automatically logs into account, selects server, and chooses character.
    If username/password are empty, uses credentials from auto_login.ini.
    """
    return mcp_client.auto_login(username=username or None, password=password or None)


@mcp.tool()
def shaiya_check_collision(start_x: float, start_y: float, start_z: float,
                           end_x: float, end_y: float, end_z: float, radius: float = 0.75) -> dict:
    """Run native collision / raycast test between two points.
    Returns whether line of sight, corridor clearance, and terrain slope are walkable.
    """
    return mcp_client.check_collision(start_x=start_x, start_y=start_y, start_z=start_z,
                                      end_x=end_x, end_y=end_y, end_z=end_z, radius=radius)


@mcp.tool()
def shaiya_get_ground_height(x: float, z: float) -> dict:
    """Query ground surface elevation Y at world coordinates (X, Z) using the native terrain raycast."""
    return mcp_client.get_ground_height(x=x, z=z)


@mcp.tool()
def shaiya_diagnose_path(goal_x: float, goal_z: float, goal_y: Optional[float] = None,
                         start_x: Optional[float] = None, start_y: Optional[float] = None, start_z: Optional[float] = None) -> dict:
    """Run and inspect pathfinder without walking. Computes waypoints from Start (or Player Pos) to Goal.
    Returns total waypoints, coordinates of each waypoint, and ground heights.
    """
    return mcp_client.diagnose_path(goal_x=goal_x, goal_z=goal_z, goal_y=goal_y,
                                    start_x=start_x, start_y=start_y, start_z=start_z)


@mcp.tool()
def shaiya_read_memory(address: str, size: int = 64) -> dict:
    """Safely read memory from the game process.
    Parameters:
      address: Hex or decimal address (e.g. '0x140A06AAC' or '140A06AAC')
      size: Number of bytes to read (default 64, max 2048)
    """
    return mcp_client.read_memory(address=address, size=size)


@mcp.tool()
def shaiya_write_memory(address: str, hex_data: str) -> dict:
    """Safely write hex bytes to the game process memory.
    Parameters:
      address: Hex or decimal address
      hex_data: Hex string to write (e.g. '90 90 90' or '01 00')
    """
    return mcp_client.write_memory(address=address, hex_data=hex_data)


@mcp.tool()
def shaiya_world_to_screen(x: float, y: float, z: float) -> dict:
    """Convert 3D world coordinates to 2D screen pixel coordinates."""
    return mcp_client.world_to_screen(x=x, y=y, z=z)


@mcp.tool()
def shaiya_toggle_menu(open_val: Optional[bool] = None) -> dict:
    """Toggle or set overlay menu visibility."""
    return mcp_client.toggle_menu(open_val=open_val)


@mcp.tool()
def shaiya_get_config() -> dict:
    """Query overlay configurations in memory."""
    return mcp_client.query_config()


@mcp.tool()
def shaiya_set_config(key: str, value: bool) -> dict:
    """Set overlay configuration (e.g. 'snaplines_enabled', 'quest_waypoints_enabled', 'menu_open')."""
    return mcp_client.set_config(key=key, value=value)


@mcp.tool()
def shaiya_unload() -> dict:
    """Unload and eject ShaiyaOverlay.dll from the game process cleanly."""
    return mcp_client.unload_dll()


@mcp.tool()
def shaiya_load(dll_path: Optional[str] = None) -> dict:
    """Inject / load ShaiyaOverlay.dll into the running game.exe process."""
    return mcp_client.load_dll(dll_path=dll_path)


if __name__ == "__main__":
    mcp.run()
