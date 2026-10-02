#pragma once

#include "Core/Types.h"
#include <cstdint>
#include <cstddef>

#ifdef MCP_TOOL

namespace ShaiyaOverlay
{
    namespace MCPBridge
    {
        bool StartServer();
        void StopServer();
        bool IsRunning();

        void OnRenderTick();
        bool RunOnRenderThreadInternal(void (*fn)(void*), void* ctx, uint32_t timeoutMs);

        template<typename T>
        inline bool RunOnRenderThread(T&& lambda, uint32_t timeoutMs = 2000)
        {
            auto thunk = [](void* ctx) {
                auto* f = reinterpret_cast<T*>(ctx);
                (*f)();
            };
            return RunOnRenderThreadInternal(thunk, reinterpret_cast<void*>(&lambda), timeoutMs);
        }

        void ProcessCommand(const char* pRequestJson, char* pResponseJson, size_t nMaxLen);
    }
}

#else

namespace ShaiyaOverlay
{
    namespace MCPBridge
    {
        inline bool StartServer() { return false; }
        inline void StopServer() { }
        inline bool IsRunning() { return false; }
        inline void OnRenderTick() { }
        template<typename T>
        inline bool RunOnRenderThread(T&&, uint32_t = 2000) { return false; }
        inline void ProcessCommand(const char*, char*, size_t) { }
    }
}

#endif // MCP_TOOL
