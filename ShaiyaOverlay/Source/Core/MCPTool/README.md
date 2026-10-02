# ShaiyaOverlay MCP Tool

Bridge de diagnóstico e controle em tempo real via **Named Pipe IPC** (`\\.\pipe\ShaiyaOverlay_MCP`) integrando o mod nativo (`ShaiyaOverlay.dll`) a clientes MCP (Model Context Protocol) via servidor FastMCP em Python.

---

## 1. Arquitetura

```text
+-----------------------+           Named Pipe IPC            +-----------------------+
|      Game Process     | <---------------------------------> |   server.py (FastMCP) |
|  (ShaiyaOverlay.dll)  |      \\.\pipe\ShaiyaOverlay_MCP     |   mcp_client.py       |
+-----------------------+                                     +-----------------------+
                                                                          ^
                                                                          | STDIO / MCP
                                                                          v
                                                              +-----------------------+
                                                              |  Claude / Cursor /    |
                                                              |  Cline / Agent MCP    |
                                                              +-----------------------+
```

1. **`ShaiyaOverlay.dll`**: Thread interna `MCPPipeThread` escuta conexões na named pipe `\\.\pipe\ShaiyaOverlay_MCP` quando compilado com a macro `MCP_TOOL`.
2. **`mcp_client.py`**: Comunicação IPC síncrona com o processo do jogo via `CallNamedPipeA` do Windows (`kernel32.dll`).
3. **`server.py`**: Servidor FastMCP que registra e expõe as ferramentas para LLMs e assistentes de IA.

---

## 2. Instalação e Teste

### Dependências Python
```powershell
pip install "mcp[cli]" fastmcp
```

### Teste de Conexão Rápido
Com o jogo aberto e a DLL injetada:
```powershell
python "G:\Games\Shaiya\default\game\ShaiyaOverlay\ShaiyaOverlay\Source\Core\MCPTool\test_mcp.py"
```

---

## 3. Configuração no Claude Desktop / Cursor / Cline

### Claude Desktop (`claude_desktop_config.json`)
Local do arquivo: `%APPDATA%\Claude\claude_desktop_config.json`

```json
{
  "mcpServers": {
    "shaiya_overlay": {
      "command": "python",
      "args": [
        "G:\\Games\\Shaiya\\default\\game\\ShaiyaOverlay\\ShaiyaOverlay\\Source\\Core\\MCPTool\\server.py"
      ]
    }
  }
}
```

---

## 4. Ferramentas Disponíveis

| Ferramenta | Descrição |
|---|---|
| `shaiya_get_status` | Status geral (PID, HP, Level, posição do jogador, status de auto-walk e menu) |
| `shaiya_get_player` | Informações detalhadas do jogador (HP, Posição, Direção, Destino, Estado, Câmera) |
| `shaiya_get_entities` | Lista monstros próximos, itens no chão e NPCs com distâncias e posições |
| `shaiya_get_nearest_target` | Encontra o monstro, item ou NPC mais próximo do jogador |
| `shaiya_get_navigation` | Retorna o status da navegação, alvo atual e todos os waypoints calculados |
| `shaiya_walk_to` | Inicia auto-walk com o pathfinder nativo A* para as coordenadas `(X, Y, Z)` |
| `shaiya_stop_walk` | Para a caminhada imediatamente |
| `shaiya_check_collision` | Testa colisão / raycast e corredor entre dois pontos no mapa |
| `shaiya_get_ground_height` | Retorna a altitude Y exata do chão em `(X, Z)` |
| `shaiya_diagnose_path` | Calcula e inspeciona toda a rota A* de waypoints sem precisar andar |
| `shaiya_read_memory` | Lê blocos de memória do jogo (retorna hex e ascii) |
| `shaiya_write_memory` | Escreve bytes na memória do jogo |
| `shaiya_world_to_screen` | Projeta coordenadas 3D para pixels 2D da tela |
| `shaiya_toggle_menu` | Abre ou fecha o menu da overlay |
| `shaiya_get_config` | Lê configurações da overlay (snaplines, quest waypoints, menu) |
| `shaiya_set_config` | Altera configurações da overlay em tempo real |
