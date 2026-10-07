#include "Camera.h"
#include "Memory.h"
#include "Game/GameOffsets.h"

namespace ShaiyaOverlay
{
    bool Camera::WorldToScreen(const Vector3& WorldPos, Vector2& OutScreen, F32 ScreenWidth, F32 ScreenHeight)
    {
        if (!Offsets.CameraEye || !Offsets.ProjMatrix || ScreenWidth <= 0.0f || ScreenHeight <= 0.0f)
            return false;

        Vector3 Eye, At, Up;
        if (!Memory::ReadBytesSafe(Offsets.CameraEye, &Eye, sizeof(Vector3)))
            return false;
        if (!Memory::ReadBytesSafe(Offsets.CameraEye + 0x0C, &At, sizeof(Vector3)))
            return false;
        if (!Memory::ReadBytesSafe(Offsets.CameraEye + 0x18, &Up, sizeof(Vector3)))
            return false;

        Matrix4x4 Proj;
        if (!Memory::ReadBytesSafe(Offsets.ProjMatrix, &Proj, sizeof(Matrix4x4)))
            return false;

        // Compute LookAtLH basis vectors
        Vector3 ZAxis(At.X - Eye.X, At.Y - Eye.Y, At.Z - Eye.Z);
        F32 ZLen = Vector3::Sqrt(ZAxis.X * ZAxis.X + ZAxis.Y * ZAxis.Y + ZAxis.Z * ZAxis.Z);
        if (ZLen <= 0.0001f)
            return false;
        ZAxis.X /= ZLen; ZAxis.Y /= ZLen; ZAxis.Z /= ZLen;

        Vector3 XAxis(
            Up.Y * ZAxis.Z - Up.Z * ZAxis.Y,
            Up.Z * ZAxis.X - Up.X * ZAxis.Z,
            Up.X * ZAxis.Y - Up.Y * ZAxis.X
        );
        F32 XLen = Vector3::Sqrt(XAxis.X * XAxis.X + XAxis.Y * XAxis.Y + XAxis.Z * XAxis.Z);
        if (XLen <= 0.0001f)
            return false;
        XAxis.X /= XLen; XAxis.Y /= XLen; XAxis.Z /= XLen;

        Vector3 YAxis(
            ZAxis.Y * XAxis.Z - ZAxis.Z * XAxis.Y,
            ZAxis.Z * XAxis.X - ZAxis.X * XAxis.Z,
            ZAxis.X * XAxis.Y - ZAxis.Y * XAxis.X
        );

        // Vector from camera Eye to WorldPos
        Vector3 Rel(WorldPos.X - Eye.X, WorldPos.Y - Eye.Y, WorldPos.Z - Eye.Z);

        F32 ViewX = Rel.X * XAxis.X + Rel.Y * XAxis.Y + Rel.Z * XAxis.Z;
        F32 ViewY = Rel.X * YAxis.X + Rel.Y * YAxis.Y + Rel.Z * YAxis.Z;
        F32 ViewZ = Rel.X * ZAxis.X + Rel.Y * ZAxis.Y + Rel.Z * ZAxis.Z;

        // Object is behind the camera
        if (ViewZ <= 0.1f)
            return false;

        // Project into clip space using row-major DirectX projection matrix
        F32 ClipX = ViewX * Proj.M[0];
        F32 ClipY = ViewY * Proj.M[5];

        F32 NdcX = ClipX / ViewZ;
        F32 NdcY = ClipY / ViewZ;

        OutScreen.X = (NdcX + 1.0f) * 0.5f * ScreenWidth;
        OutScreen.Y = (1.0f - NdcY) * 0.5f * ScreenHeight;

        return true;
    }
}
