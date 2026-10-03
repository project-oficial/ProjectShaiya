#include "GameOffsets.h"
#include "Core/PatternScanner.h"
#include "Core/Memory.h"
#include "Core/Logger.h"

namespace ShaiyaOverlay
{
    GameOffsets Offsets = { 0 };

    bool GameOffsets::Initialize()
    {
        U64 ImageBase = Memory::GetModuleBase(nullptr);
        Logger::Info("GameOffsets: Resolving addresses via runtime Pattern Scan (ImageBase: 0x%llX)...", ImageBase);

        // CWorldMgr member offsets
        Offsets.CharacterMapOffset = 0x196338;
        Offsets.MonsterMapOffset   = 0x196438;
        Offsets.ItemMapOffset      = 0x1963D8;
        Offsets.NpcMapOffset       = 0x196478;
        Offsets.LocalPlayerPtrOffset = 0x19C0A0;
        Offsets.WorldMgrMoveFlagOffset = 0x19CD78;

        // CMonster offsets
        Offsets.MonsterPosX        = 0x08;
        Offsets.MonsterPosY        = 0x0C;
        Offsets.MonsterPosZ        = 0x10;
        Offsets.MonsterWorldId     = 0x2C;
        Offsets.MonsterMobId       = 0x30;
        Offsets.MonsterLevel       = 0x34;
        Offsets.MonsterMaxHp       = 0x110;
        Offsets.MonsterCurrentHp   = 0x114;

        // CNpc offsets
        Offsets.NpcPosX            = 0x6C;
        Offsets.NpcPosY            = 0x70;
        Offsets.NpcPosZ            = 0x74;
        Offsets.NpcName            = 0x08;

        // CCharacter (Player) offsets
        Offsets.PlayerPosX         = 0x54;
        Offsets.PlayerPosY         = 0x58;
        Offsets.PlayerPosZ         = 0x5C;
        Offsets.PlayerDirX         = 0x60;
        Offsets.PlayerDirY         = 0x64;
        Offsets.PlayerDirZ         = 0x68;
        Offsets.PlayerDestX        = 0x32C;
        Offsets.PlayerDestY        = 0x330;
        Offsets.PlayerDestZ        = 0x334;
        Offsets.PlayerState        = 0x2B4;
        Offsets.PlayerIdOffset     = 0x78;
        Offsets.PlayerTargetWorldId = 0x344;

        // CItem (Ground loot) offsets
        Offsets.ItemWorldId        = 0x08;
        Offsets.ItemOwnerId        = 0x14;
        Offsets.ItemDespawnTime    = 0x18;
        Offsets.ItemPosX           = 0x40;
        Offsets.ItemPosY           = 0x44;
        Offsets.ItemPosZ           = 0x48;
        Offsets.ItemType           = 0x6C;
        Offsets.ItemTypeId         = 0x6D;
        Offsets.ItemCount          = 0x70;
        Offsets.ItemQuality        = 0x75;

        // 1. WorldManager
        U64 MatchWorld = PatternScanner::ScanModule(nullptr, "48 8D 0D ? ? ? ? E8 ? ? ? ? 41 B8 09 00 00 00 48 8D 15");
        Offsets.WorldManager = MatchWorld ? PatternScanner::RipRelative(MatchWorld, 3, 7) : 0;
        if (Offsets.WorldManager)
            Logger::Info("  WorldManager    : 0x%llX", Offsets.WorldManager);
        else
            Logger::Error("  WorldManager    : FAILED (AOB not found)");

        // 2. PlayerId
        U64 MatchPlayerId = PatternScanner::ScanModule(nullptr, "8B 05 ? ? ? ? 39 43 78 75 11 83 3D");
        Offsets.PlayerId = MatchPlayerId ? PatternScanner::RipRelative(MatchPlayerId, 2, 6) : 0;
        if (Offsets.PlayerId)
            Logger::Info("  PlayerId        : 0x%llX", Offsets.PlayerId);
        else
            Logger::Error("  PlayerId        : FAILED (AOB not found)");

        // 3. PlayerStats (HP, MP, SP, and Level)
        U64 MatchHudVitals = PatternScanner::ScanModule(nullptr, "8B 0D ? ? ? ? 89 8B 14 03 00 00 8B 05");
        if (MatchHudVitals)
        {
            Offsets.PlayerMaxHp     = PatternScanner::RipRelative(MatchHudVitals, 2, 6);
            Offsets.PlayerLevel     = Offsets.PlayerMaxHp ? (Offsets.PlayerMaxHp - 4) : 0;
            Offsets.PlayerMaxMp     = Offsets.PlayerMaxHp ? (Offsets.PlayerMaxHp + 4) : 0;
            Offsets.PlayerMaxSp     = Offsets.PlayerMaxHp ? (Offsets.PlayerMaxHp + 8) : 0;

            Offsets.PlayerCurrentHp = PatternScanner::RipRelative(MatchHudVitals + 0x32, 2, 6);
            Offsets.PlayerCurrentMp = Offsets.PlayerCurrentHp ? (Offsets.PlayerCurrentHp + 4) : 0;
            Offsets.PlayerCurrentSp = Offsets.PlayerCurrentHp ? (Offsets.PlayerCurrentHp + 8) : 0;

            Logger::Info("  PlayerCurrentHp : 0x%llX", Offsets.PlayerCurrentHp);
            Logger::Info("  PlayerMaxHp     : 0x%llX", Offsets.PlayerMaxHp);
            Logger::Info("  PlayerCurrentMp : 0x%llX", Offsets.PlayerCurrentMp);
            Logger::Info("  PlayerMaxMp     : 0x%llX", Offsets.PlayerMaxMp);
            Logger::Info("  PlayerCurrentSp : 0x%llX", Offsets.PlayerCurrentSp);
            Logger::Info("  PlayerMaxSp     : 0x%llX", Offsets.PlayerMaxSp);
            Logger::Info("  PlayerLevel     : 0x%llX", Offsets.PlayerLevel);
        }
        else
        {
            Offsets.PlayerCurrentHp = 0;
            Offsets.PlayerMaxHp     = 0;
            Offsets.PlayerCurrentMp = 0;
            Offsets.PlayerMaxMp     = 0;
            Offsets.PlayerCurrentSp = 0;
            Offsets.PlayerMaxSp     = 0;
            Offsets.PlayerLevel     = 0;
            Logger::Error("  PlayerStats     : FAILED (AOB not found)");
        }

        // 4. SkillVector
        U64 MatchSkill = PatternScanner::ScanModule(nullptr, "75 DC 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 4C 89 25 ? ? ? ? 0F 57 C0 66 0F 7F 05");
        Offsets.SkillVector = MatchSkill ? PatternScanner::RipRelative(MatchSkill, 12, 16) : 0;
        if (Offsets.SkillVector)
            Logger::Info("  SkillVector     : 0x%llX", Offsets.SkillVector);
        else
            Logger::Error("  SkillVector     : FAILED (AOB not found)");

        // 5. D3DDevice
        U64 MatchD3D = PatternScanner::ScanModule(nullptr, "48 89 05 ? ? ? ? 48 89 05 ? ? ? ? EB 14 89 35");
        Offsets.D3DDevice = MatchD3D ? PatternScanner::RipRelative(MatchD3D, 3, 7) : 0;
        if (Offsets.D3DDevice)
            Logger::Info("  D3DDevice       : 0x%llX", Offsets.D3DDevice);
        else
            Logger::Error("  D3DDevice       : FAILED (AOB not found)");

        // 6. GameHwnd
        U64 MatchHwnd = PatternScanner::ScanModule(nullptr, "48 8B 0D ? ? ? ? FF 15 ? ? ? ? 33 C0 E9 ? ? ? ? 48 8B 05");
        Offsets.GameHwnd = MatchHwnd ? PatternScanner::RipRelative(MatchHwnd, 3, 7) : 0;
        if (Offsets.GameHwnd)
            Logger::Info("  GameHwnd        : 0x%llX", Offsets.GameHwnd);
        else
            Logger::Error("  GameHwnd        : FAILED (AOB not found)");

        // 7. QuickSlotBase
        U64 MatchQuick = PatternScanner::ScanModule(nullptr, "48 8D 05 ? ? ? ? 48 8D 15 ? ? ? ? 39 08 74 0B 48 83 C0 14");
        if (MatchQuick)
        {
            U64 QuickDword = PatternScanner::RipRelative(MatchQuick, 3, 7);
            Offsets.QuickSlotBase = QuickDword ? (QuickDword - 4) : 0;
            Logger::Info("  QuickSlotBase   : 0x%llX", Offsets.QuickSlotBase);
        }
        else
        {
            Offsets.QuickSlotBase = 0;
            Logger::Error("  QuickSlotBase   : FAILED (AOB not found)");
        }

        // 8. ItemDb
        U64 MatchItemDb = PatternScanner::ScanModule(nullptr, "48 8D 0D ? ? ? ? 0F B6 50 6C E8 ? ? ? ? 48 85 C0 0F 84");
        Offsets.ItemDb = MatchItemDb ? PatternScanner::RipRelative(MatchItemDb, 3, 7) : 0;
        if (Offsets.ItemDb)
            Logger::Info("  ItemDb          : 0x%llX", Offsets.ItemDb);
        else
            Logger::Error("  ItemDb          : FAILED (AOB not found)");

        // 9. CameraEye
        U64 MatchCam = PatternScanner::ScanModule(nullptr, "4C 8D 0D ? ? ? ? 4C 8D 05 ? ? ? ? 48 8B D3 48 8D 8D");
        if (MatchCam)
        {
            U64 TargetVec = PatternScanner::RipRelative(MatchCam + 7, 3, 7);
            Offsets.CameraEye = TargetVec ? (TargetVec - 12) : 0;
            Logger::Info("  CameraEye       : 0x%llX", Offsets.CameraEye);
        }
        else
        {
            Offsets.CameraEye = 0;
            Logger::Error("  CameraEye       : FAILED (AOB not found)");
        }

        // 10. ProjMatrix
        U64 MatchProj = PatternScanner::ScanModule(nullptr, "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 85 C9 74 15 48 8B 01 4C 8D 05");
        Offsets.ProjMatrix = MatchProj ? PatternScanner::RipRelative(MatchProj, 3, 7) : 0;
        if (Offsets.ProjMatrix)
            Logger::Info("  ProjMatrix      : 0x%llX", Offsets.ProjMatrix);
        else
            Logger::Error("  ProjMatrix      : FAILED (AOB not found)");

        // 11. QuestVector
        Offsets.QuestVector = Offsets.SkillVector ? (Offsets.SkillVector + 0x28) : 0;
        if (Offsets.QuestVector)
            Logger::Info("  QuestVector     : 0x%llX", Offsets.QuestVector);
        else
            Logger::Error("  QuestVector     : FAILED (SkillVector not resolved)");

        // 12. NpcFile (Quest and NPC static database)
        U64 MatchNpcFile = PatternScanner::ScanModule(nullptr, "48 8D 0D ? ? ? ? E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 33 D2 41 B8 00 01 00 00");
        Offsets.NpcFile = MatchNpcFile ? PatternScanner::RipRelative(MatchNpcFile, 3, 7) : 0;
        if (Offsets.NpcFile)
            Logger::Info("  NpcFile         : 0x%llX", Offsets.NpcFile);
        else
            Logger::Error("  NpcFile         : FAILED (AOB not found)");

        // 13. QuestTextTable
        U64 MatchTxt = PatternScanner::ScanModule(nullptr, "4C 69 C1 E8 01 00 00 4C 8B 35 ? ? ? ? 49 81 C6 18 FE FF FF");
        Offsets.QuestTextTablePtr = MatchTxt ? PatternScanner::RipRelative(MatchTxt + 7, 3, 7) : 0;
        Offsets.QuestTextTable = 0;
        if (Offsets.QuestTextTablePtr)
            Memory::ReadSafe(Offsets.QuestTextTablePtr, &Offsets.QuestTextTable);

        if (Offsets.QuestTextTablePtr)
            Logger::Info("  QuestTextTable  : 0x%llX (Ptr: 0x%llX)", Offsets.QuestTextTable, Offsets.QuestTextTablePtr);
        else
            Logger::Error("  QuestTextTable  : FAILED (AOB not found)");

        // 14. QuestMarkerList
        U64 MatchMarkers = PatternScanner::ScanModule(nullptr, "4C 8D 25 ? ? ? ? 66 41 0F 6E F5 0F 5B F6");
        Offsets.QuestMarkerList = MatchMarkers ? PatternScanner::RipRelative(MatchMarkers, 3, 7) : 0;
        if (Offsets.QuestMarkerList)
            Logger::Info("  QuestMarkerList : 0x%llX", Offsets.QuestMarkerList);
        else
            Logger::Error("  QuestMarkerList : FAILED (AOB not found)");

        // 15. SetAction (sub_14005F520)
        U64 MatchSetAction = PatternScanner::ScanModule(nullptr, "48 89 5C 24 08 57 48 83 EC 20 83 B9 AC 04 00 00 00");
        Offsets.SetActionAddr = MatchSetAction;
        if (Offsets.SetActionAddr)
            Logger::Info("  SetActionAddr   : 0x%llX", Offsets.SetActionAddr);
        else
            Logger::Error("  SetActionAddr   : FAILED (AOB not found)");

        // 16. SendMovePacket (sub_1402E6830)
        U64 MatchMovePacket = PatternScanner::ScanModule(nullptr, "48 8B C4 53 48 81 EC B0 00 00 00 48 8B 99 A0 C0 19 00");
        Offsets.SendMovePacketAddr = MatchMovePacket;
        if (Offsets.SendMovePacketAddr)
            Logger::Info("  SendMovePacket  : 0x%llX", Offsets.SendMovePacketAddr);
        else
            Logger::Error("  SendMovePacket  : FAILED (AOB not found)");

        // 17. PlayerInventory (byte_140A03F00)
        U64 MatchInv = PatternScanner::ScanModule(nullptr, "48 8D 0D ? ? ? ? 44 0F B6 47 64");
        Offsets.PlayerInventory = MatchInv ? PatternScanner::RipRelative(MatchInv, 3, 7) : 0;
        if (Offsets.PlayerInventory)
            Logger::Info("  PlayerInventory : 0x%llX", Offsets.PlayerInventory);
        else
            Logger::Error("  PlayerInventory : FAILED (AOB not found)");

        // 18. KeyBuffer (byte_142FD0F60)
        U64 MatchKeyBuf = PatternScanner::ScanModule(nullptr, "C0 E1 07 4C 8D 0D ? ? ? ?");
        Offsets.KeyBuffer = MatchKeyBuf ? PatternScanner::RipRelative(MatchKeyBuf + 3, 3, 7) : 0;
        if (Offsets.KeyBuffer)
            Logger::Info("  KeyBuffer       : 0x%llX", Offsets.KeyBuffer);
        else
            Logger::Error("  KeyBuffer       : FAILED (AOB not found)");

        // 19. CheckLineOfSight (sub_1402DA2F0)
        U64 MatchCheckLOS = PatternScanner::ScanModule(nullptr, "48 8B C4 48 89 58 10 48 89 78 18 55 48 8D 68 A8 48 81 EC 50 01 00 00");
        Offsets.CheckLineOfSightAddr = MatchCheckLOS;
        if (Offsets.CheckLineOfSightAddr)
        {
            Logger::Info("  CheckLineOfSight: 0x%llX", Offsets.CheckLineOfSightAddr);
            U8* PatchPtr = reinterpret_cast<U8*>(Offsets.CheckLineOfSightAddr + 0x1ED);
            if (PatchPtr[0] == 0xB9 && PatchPtr[1] == 0xC9 && PatchPtr[2] == 0x01)
            {
                DWORD OldProtect = 0;
                if (VirtualProtect(PatchPtr, 5, PAGE_EXECUTE_READWRITE, &OldProtect))
                {
                    // EB 36 90 90 90: jmp +0x36 straight to xor eax, eax; jmp loc_1402DB366
                    PatchPtr[0] = 0xEB;
                    PatchPtr[1] = 0x36;
                    PatchPtr[2] = 0x90;
                    PatchPtr[3] = 0x90;
                    PatchPtr[4] = 0x90;
                    VirtualProtect(PatchPtr, 5, OldProtect, &OldProtect);
                    Logger::Info("  CheckLineOfSight: Silenced UI error sound & chat spam successfully.");
                }
            }
        }
        else
        {
            Logger::Error("  CheckLineOfSight: FAILED (AOB not found)");
        }

        // 20. GetGroundHeight (sub_14005C320)
        U64 MatchHeight = PatternScanner::ScanModule(nullptr, "48 8B C4 48 83 EC 58 80 79 09 00 74 72");
        Offsets.GetGroundHeightAddr = MatchHeight;
        if (Offsets.GetGroundHeightAddr)
            Logger::Info("  GetGroundHeight : 0x%llX", Offsets.GetGroundHeightAddr);
        else
            Logger::Error("  GetGroundHeight : FAILED (AOB not found)");

        // 21. CastSkill (sub_140385C80)
        U64 MatchCast = PatternScanner::ScanModule(nullptr, "B8 28 10 00 00 E8 ? ? ? ? 48 2B E0 B8 17 05 00 00");
        Offsets.CastSkillAddr = MatchCast;
        if (Offsets.CastSkillAddr)
            Logger::Info("  CastSkillAddr   : 0x%llX", Offsets.CastSkillAddr);
        else
            Logger::Error("  CastSkillAddr   : FAILED (AOB not found)");

        // 22. GetGameTimeMs (sub_14039CF70)
        U64 MatchTime = PatternScanner::ScanModule(nullptr, "39 6B 10 74 ? E8 ? ? ? ? 41 8B 8F 70 68 00 00");
        Offsets.GetGameTimeMsAddr = MatchTime ? PatternScanner::RipRelative(MatchTime + 5, 1, 5) : 0;
        if (Offsets.GetGameTimeMsAddr)
            Logger::Info("  GetGameTimeMs   : 0x%llX", Offsets.GetGameTimeMsAddr);
        else
            Logger::Error("  GetGameTimeMs   : FAILED (AOB not found)");

        bool Success = Offsets.WorldManager &&
                       Offsets.PlayerId &&
                       Offsets.PlayerCurrentHp &&
                       Offsets.PlayerMaxHp &&
                       Offsets.PlayerLevel &&
                       Offsets.SkillVector &&
                       Offsets.D3DDevice &&
                       Offsets.GameHwnd &&
                       Offsets.QuickSlotBase &&
                       Offsets.ItemDb &&
                       Offsets.CameraEye &&
                       Offsets.ProjMatrix &&
                       Offsets.QuestVector &&
                       Offsets.NpcFile &&
                       Offsets.QuestTextTablePtr &&
                       Offsets.QuestMarkerList &&
                       Offsets.SetActionAddr &&
                       Offsets.SendMovePacketAddr &&
                       Offsets.PlayerInventory &&
                       Offsets.KeyBuffer &&
                       Offsets.CheckLineOfSightAddr &&
                       Offsets.GetGroundHeightAddr &&
                       Offsets.CastSkillAddr &&
                       Offsets.GetGameTimeMsAddr;

        if (!Success)
            Logger::Error("GameOffsets: One or more AOB patterns FAILED to resolve!");
        else
            Logger::Info("GameOffsets: All 22 AOB patterns resolved successfully.");

        return Success;
    }
}
