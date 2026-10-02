import sys
import json
import mcp_client

def main():
    print("Testing ShaiyaOverlay Named Pipe (\\\\.\\pipe\\ShaiyaOverlay_MCP)...")
    connected = mcp_client.is_game_connected()
    print(f"Game Connected: {connected}")

    status = mcp_client.query_status()
    print("\n--- Status ---")
    print(json.dumps(status, indent=2))

    if not status.get("game_connected"):
        print("\n[!] Game or overlay not running yet. Inject DLL to test live.")
        return

    player = mcp_client.query_player()
    print("\n--- Player ---")
    print(json.dumps(player, indent=2))

    nav = mcp_client.query_navigation()
    print("\n--- Navigation ---")
    print(json.dumps(nav, indent=2))

if __name__ == "__main__":
    main()
