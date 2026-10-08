#pragma once

struct ID3D11Device;
struct SharedTheme;

namespace BladeBridge
{
    bool Initialize();
    bool CreateDeviceObjects(ID3D11Device* device);
    void ReleaseDeviceObjects();
    void ApplyWhiteLabel(const SharedTheme& theme);
    void SetSession(const char* user, const char* time_left);
    void Render();
}
