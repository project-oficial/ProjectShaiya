#include "GameOffsets.h"
#include "Core/PatternScanner.h"
#include "Core/Memory.h"
#include "Core/Logger.h"

namespace ShaiyaOverlay
{
    GameOffsets Offsets = { 0 };

    void GameOffsets::Initialize()
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
        if (MatchWorld)
            Offsets.WorldManager = PatternScanner::RipRelative(MatchWorld, 3, 7);
        else
            Offsets.WorldManager = ImageBase + 0x23D9EB0;
        Logger::Info("  WorldManager    : 0x%llX (AOB: %s)", Offsets.WorldManager, MatchWorld ? "YES" : "FALLBACK");

        // 2. PlayerId
        U64 MatchPlayerId = PatternScanner::ScanModule(nullptr, "8B 05 ? ? ? ? 39 43 78 75 11 83 3D");
        if (MatchPlayerId)
            Offsets.PlayerId = PatternScanner::RipRelative(MatchPlayerId, 2, 6);
        else
            Offsets.PlayerId = ImageBase + 0x0A06AAC;
        Logger::Info("  PlayerId        : 0x%llX (AOB: %s)", Offsets.PlayerId, MatchPlayerId ? "YES" : "FALLBACK");

        // 3. PlayerStats (HP and Level)
        U64 MatchStats = PatternScanner::ScanModule(nullptr, "44 89 3D ? ? ? ? 44 89 79 44 4C 8B 35");
        if (MatchStats)
        {
            U64 StatsBase = PatternScanner::RipRelative(MatchStats, 3, 7);
            Offsets.PlayerCurrentHp = StatsBase + 4;
            Offsets.PlayerMaxHp     = StatsBase + 8;
            Offsets.PlayerLevel     = StatsBase - 0x48;
        }
        else
        {
            Offsets.PlayerLevel     = ImageBase + 0x0A10F70;
            Offsets.PlayerCurrentHp = ImageBase + 0x0A10F74;
            Offsets.PlayerMaxHp     = ImageBase + 0x0A10F74;
        }
        Logger::Info("  PlayerCurrentHp : 0x%llX (AOB: %s)", Offsets.PlayerCurrentHp, MatchStats ? "YES" : "FALLBACK");
        Logger::Info("  PlayerMaxHp     : 0x%llX (AOB: %s)", Offsets.PlayerMaxHp, MatchStats ? "YES" : "FALLBACK");
        Logger::Info("  PlayerLevel     : 0x%llX (AOB: %s)", Offsets.PlayerLevel, MatchStats ? "YES" : "FALLBACK");

        // 4. SkillVector
        U64 MatchSkill = PatternScanner::ScanModule(nullptr, "75 DC 48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? 4C 89 25 ? ? ? ? 0F 57 C0 66 0F 7F 05");
        if (MatchSkill)
            Offsets.SkillVector = PatternScanner::RipRelative(MatchSkill, 12, 16);
        else
            Offsets.SkillVector = ImageBase + 0x23CC2E0;
        Logger::Info("  SkillVector     : 0x%llX (AOB: %s)", Offsets.SkillVector, MatchSkill ? "YES" : "FALLBACK");

        // 5. D3DDevice
        U64 MatchD3D = PatternScanner::ScanModule(nullptr, "48 89 05 ? ? ? ? 48 89 05 ? ? ? ? EB 14 89 35");
        if (MatchD3D)
            Offsets.D3DDevice = PatternScanner::RipRelative(MatchD3D, 3, 7);
        else
            Offsets.D3DDevice = ImageBase + 0x09DF3E0;
        Logger::Info("  D3DDevice       : 0x%llX (AOB: %s)", Offsets.D3DDevice, MatchD3D ? "YES" : "FALLBACK");

        // 6. GameHwnd
        U64 MatchHwnd = PatternScanner::ScanModule(nullptr, "48 8B 0D ? ? ? ? FF 15 ? ? ? ? 33 C0 E9 ? ? ? ? 48 8B 05");
        if (MatchHwnd)
            Offsets.GameHwnd = PatternScanner::RipRelative(MatchHwnd, 3, 7);
        else
            Offsets.GameHwnd = ImageBase + 0x0840D48;
        Logger::Info("  GameHwnd        : 0x%llX (AOB: %s)", Offsets.GameHwnd, MatchHwnd ? "YES" : "FALLBACK");

        // 7. QuickSlotBase
        U64 MatchQuick = PatternScanner::ScanModule(nullptr, "48 8D 05 ? ? ? ? 48 8D 15 ? ? ? ? 39 08 74 0B 48 83 C0 14");
        if (MatchQuick)
        {
            U64 QuickDword = PatternScanner::RipRelative(MatchQuick, 3, 7);
            Offsets.QuickSlotBase = QuickDword - 4;
        }
        else
        {
            Offsets.QuickSlotBase = ImageBase + 0x23CC53C;
        }
        Logger::Info("  QuickSlotBase   : 0x%llX (AOB: %s)", Offsets.QuickSlotBase, MatchQuick ? "YES" : "FALLBACK");

        // 8. ItemDb
        U64 MatchItemDb = PatternScanner::ScanModule(nullptr, "48 8D 0D ? ? ? ? 0F B6 50 6C E8 ? ? ? ? 48 85 C0 0F 84");
        if (MatchItemDb)
            Offsets.ItemDb = PatternScanner::RipRelative(MatchItemDb, 3, 7);
        else
            Offsets.ItemDb = ImageBase + 0x0A3C580;
        Logger::Info("  ItemDb          : 0x%llX (AOB: %s)", Offsets.ItemDb, MatchItemDb ? "YES" : "FALLBACK");

        // 9. CameraEye
        U64 MatchCam = PatternScanner::ScanModule(nullptr, "4C 8D 0D ? ? ? ? 4C 8D 05 ? ? ? ? 48 8B D3 48 8D 8D");
        if (MatchCam)
        {
            U64 TargetVec = PatternScanner::RipRelative(MatchCam + 7, 3, 7);
            Offsets.CameraEye = TargetVec - 12;
        }
        else
        {
            Offsets.CameraEye = ImageBase + 0x07F8520;
        }
        Logger::Info("  CameraEye       : 0x%llX (AOB: %s)", Offsets.CameraEye, MatchCam ? "YES" : "FALLBACK");

        // 10. ProjMatrix
        U64 MatchProj = PatternScanner::ScanModule(nullptr, "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 85 C9 74 15 48 8B 01 4C 8D 05");
        if (MatchProj)
            Offsets.ProjMatrix = PatternScanner::RipRelative(MatchProj, 3, 7);
        else
            Offsets.ProjMatrix = ImageBase + 0x09DEFD0;
        Logger::Info("  ProjMatrix      : 0x%llX (AOB: %s)", Offsets.ProjMatrix, MatchProj ? "YES" : "FALLBACK");

        // 11. QuestVector
        Offsets.QuestVector = Offsets.SkillVector ? (Offsets.SkillVector + 0x28) : (ImageBase + 0x23CC308);
        Logger::Info("  QuestVector     : 0x%llX", Offsets.QuestVector);

        // 12. NpcFile (Quest and NPC static database)
        U64 MatchNpcFile = PatternScanner::ScanModule(nullptr, "48 8D 0D ? ? ? ? E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 33 D2 41 B8 00 01 00 00");
        if (MatchNpcFile)
            Offsets.NpcFile = PatternScanner::RipRelative(MatchNpcFile, 3, 7);
        else
            Offsets.NpcFile = ImageBase + 0x0A3C9B0;
        Logger::Info("  NpcFile         : 0x%llX (AOB: %s)", Offsets.NpcFile, MatchNpcFile ? "YES" : "FALLBACK");

        // 13. QuestTextTable
        U64 MatchTxt = PatternScanner::ScanModule(nullptr, "4C 69 C1 E8 01 00 00 4C 8B 35 ? ? ? ? 49 81 C6 18 FE FF FF");
        U64 TxtPtrAddr = 0;
        if (MatchTxt)
            TxtPtrAddr = PatternScanner::RipRelative(MatchTxt + 7, 3, 7);
        else
            TxtPtrAddr = ImageBase + 0x0A3CA88;

        Offsets.QuestTextTable = 0;
        if (TxtPtrAddr)
            Memory::ReadSafe(TxtPtrAddr, &Offsets.QuestTextTable);
        Logger::Info("  QuestTextTable  : 0x%llX (Ptr: 0x%llX, AOB: %s)", Offsets.QuestTextTable, TxtPtrAddr, MatchTxt ? "YES" : "FALLBACK");

        // 14. QuestMarkerList
        U64 MatchMarkers = PatternScanner::ScanModule(nullptr, "4C 8D 25 ? ? ? ? 66 41 0F 6E F5 0F 5B F6");
        if (MatchMarkers)
            Offsets.QuestMarkerList = PatternScanner::RipRelative(MatchMarkers, 3, 7);
        else
            Offsets.QuestMarkerList = ImageBase + 0x23D9C80;
        Logger::Info("  QuestMarkerList : 0x%llX (AOB: %s)", Offsets.QuestMarkerList, MatchMarkers ? "YES" : "FALLBACK");

        // 15. SetAction (sub_14005F520)
        U64 MatchSetAction = PatternScanner::ScanModule(nullptr, "48 89 5C 24 08 57 48 83 EC 20 83 B9 AC 04 00 00 00");
        Offsets.SetActionAddr = MatchSetAction ? MatchSetAction : (ImageBase + 0x05F520);
        Logger::Info("  SetActionAddr   : 0x%llX (AOB: %s)", Offsets.SetActionAddr, MatchSetAction ? "YES" : "FALLBACK");

        // 16. SendMovePacket (sub_1402E6830)
        U64 MatchMovePacket = PatternScanner::ScanModule(nullptr, "48 8B C4 53 48 81 EC B0 00 00 00 48 8B 99 A0 C0 19 00");
        Offsets.SendMovePacketAddr = MatchMovePacket ? MatchMovePacket : (ImageBase + 0x2E6830);
        Logger::Info("  SendMovePacket  : 0x%llX (AOB: %s)", Offsets.SendMovePacketAddr, MatchMovePacket ? "YES" : "FALLBACK");

        // 17. PlayerInventory (byte_140A03F00)
        U64 MatchInv = PatternScanner::ScanModule(nullptr, "48 8D 0D ? ? ? ? 44 0F B6 47 64");
        if (MatchInv)
            Offsets.PlayerInventory = PatternScanner::RipRelative(MatchInv, 3, 7);
        else
            Offsets.PlayerInventory = ImageBase + 0x0A03F00;
        Logger::Info("  PlayerInventory : 0x%llX (AOB: %s)", Offsets.PlayerInventory, MatchInv ? "YES" : "FALLBACK");

        // 18. KeyBuffer (byte_142FD0F60)
        U64 MatchKeyBuf = PatternScanner::ScanModule(nullptr, "C0 E1 07 4C 8D 0D ? ? ? ?");
        if (MatchKeyBuf)
            Offsets.KeyBuffer = PatternScanner::RipRelative(MatchKeyBuf + 3, 3, 7);
        else
            Offsets.KeyBuffer = ImageBase + 0x2FD0F60;
        Logger::Info("  KeyBuffer       : 0x%llX (AOB: %s)", Offsets.KeyBuffer, MatchKeyBuf ? "YES" : "FALLBACK");

        // 19. CheckLineOfSight (sub_1402DA2F0)
        U64 MatchCheckLOS = PatternScanner::ScanModule(nullptr, "48 8B C4 48 89 58 10 48 89 78 18 55 48 8D 68 A8 48 81 EC 50 01 00 00");
        Offsets.CheckLineOfSightAddr = MatchCheckLOS ? MatchCheckLOS : (ImageBase + 0x2DA2F0);
        Logger::Info("  CheckLineOfSight: 0x%llX (AOB: %s)", Offsets.CheckLineOfSightAddr, MatchCheckLOS ? "YES" : "FALLBACK");

        // Silence error sound (ui_error001.wav) and chat spam when CheckLineOfSight hits obstacles
        if (Offsets.CheckLineOfSightAddr)
        {
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

        // 20. GetGroundHeight (sub_14005C320)
        U64 MatchHeight = PatternScanner::ScanModule(nullptr, "48 8B C4 48 83 EC 58 80 79 09 00 74 72");
        Offsets.GetGroundHeightAddr = MatchHeight ? MatchHeight : (ImageBase + 0x05C320);
        Logger::Info("  GetGroundHeight : 0x%llX (AOB: %s)", Offsets.GetGroundHeightAddr, MatchHeight ? "YES" : "FALLBACK");

        // 21. CastSkill (sub_140385C80)
        U64 MatchCast = PatternScanner::ScanModule(nullptr, "B8 28 10 00 00 E8 ? ? ? ? 48 2B E0 B8 17 05 00 00");
        Offsets.CastSkillAddr = MatchCast ? MatchCast : (ImageBase + 0x385C80);
        Logger::Info("  CastSkillAddr   : 0x%llX (AOB: %s)", Offsets.CastSkillAddr, MatchCast ? "YES" : "FALLBACK");
    }
}
