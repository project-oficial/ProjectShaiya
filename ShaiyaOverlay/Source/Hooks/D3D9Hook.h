#pragma once

#include "Core/Types.h"
#include <d3d9.h>

namespace ShaiyaOverlay
{
    using EndSceneFn = HRESULT(__fastcall*)(IDirect3DDevice9*);
    using ResetFn = HRESULT(__fastcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

    class D3D9Hook
    {
    public:
        static bool Initialize();
        static void Uninitialize();

        static IDirect3DDevice9* GetDevice() { return Device; }

    private:
        static HRESULT __fastcall HookedEndScene(IDirect3DDevice9* InDevice);
        static HRESULT __fastcall HookedReset(IDirect3DDevice9* InDevice, D3DPRESENT_PARAMETERS* Params);

        static void** DeviceVTable;
        static EndSceneFn OriginalEndScene;
        static ResetFn OriginalReset;

        static IDirect3DDevice9* Device;
        static bool IsInitialized;
        static bool IsHooked;
        static volatile LONG ActiveEndSceneCalls;
    };
}
