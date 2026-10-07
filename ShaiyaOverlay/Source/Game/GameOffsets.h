#pragma once

#include "Core/Types.h"

namespace ShaiyaOverlay
{
    struct GameOffsets
    {
        // Resolved runtime addresses
        U64 WorldManager;
        U64 PlayerId;
        U64 PlayerLevel;
        U64 PlayerCurrentHp;
        U64 PlayerMaxHp;
        U64 PlayerCurrentMp;
        U64 PlayerMaxMp;
        U64 PlayerCurrentSp;
        U64 PlayerMaxSp;
        U64 SkillVector;
        U64 D3DDevice;
        U64 GameHwnd;
        U64 QuickSlotBase;
        U64 ItemDb;
        U64 CameraEye;
        U64 ProjMatrix;
        U64 QuestVector;
        U64 NpcFile;
        U64 QuestTextTable;
        U64 QuestTextTablePtr;
        U64 QuestMarkerList;
        U64 RadarCountAddr;
        U64 RadarArrayAddr;
        U64 PlayerInventory;
        U64 SetActionAddr;
        U64 SendMovePacketAddr;
        U64 KeyBuffer;
        U64 CheckLineOfSightAddr;
        U64 GetGroundHeightAddr;
        U64 CastSkillAddr;
        U64 GetGameTimeMsAddr;
        U64 GameStateAddr;
        U64 LoginPtr;
        U64 CharacterSelectPtr;
        U64 NetworkPtr;
        U64 HandshakeStatusAddr;
        U64 SubmitLoginAddr;
        U64 ConfirmServerAddr;
        U64 SelectSlotAddr;
        U64 GetItemRecordAddr;
        U64 QuestMobSet;
        U64 MainUIPtr;
        U64 GetSkillRecordAddr;
        U64 SendPickUpAddr;
        U64 SendCharBuffPacketAddr;
        U64 SendUseItemAddr;
        U64 TargetTypeAddr;

        // CWorldMgr offsets
        U32 CharacterMapOffset;
        U32 MonsterMapOffset;
        U32 ItemMapOffset;
        U32 NpcMapOffset;
        U32 LocalPlayerPtrOffset;
        U32 WorldMgrMoveFlagOffset;

        // CMonster offsets
        U32 MonsterPosX;
        U32 MonsterPosY;
        U32 MonsterPosZ;
        U32 MonsterWorldId;
        U32 MonsterMobId;
        U32 MonsterLevel;
        U32 MonsterMaxHp;
        U32 MonsterCurrentHp;

        // CNpc offsets
        U32 NpcPosX;
        U32 NpcPosY;
        U32 NpcPosZ;
        U32 NpcName;

        // CCharacter (Player) offsets
        U32 PlayerPosX;
        U32 PlayerPosY;
        U32 PlayerPosZ;
        U32 PlayerDirX;
        U32 PlayerDirY;
        U32 PlayerDirZ;
        U32 PlayerDestX;
        U32 PlayerDestY;
        U32 PlayerDestZ;
        U32 PlayerMoveDirX;
        U32 PlayerMoveDirY;
        U32 PlayerMoveDirZ;
        U32 PlayerState;
        U32 PlayerIdOffset;
        U32 PlayerTargetWorldId;

        // CItem offsets (ground dropped loot)
        U32 ItemWorldId;
        U32 ItemOwnerId;
        U32 ItemDespawnTime;
        U32 ItemPosX;
        U32 ItemPosY;
        U32 ItemPosZ;
        U32 ItemType;
        U32 ItemTypeId;
        U32 ItemCount;
        U32 ItemQuality;

        // Inventory offsets
        U32 PlayerInventoryBagsOffset;
        U32 InventorySlotStride;

        // ItemDb & ItemRecord offsets
        U32 ItemDbSkillMapBaseOffset;
        U32 ItemRecordHpRecovery;
        U32 ItemRecordMpRecovery;
        U32 ItemRecordSpRecovery;
        U32 SkillRecordCategory;
        U32 SkillRecordTargetType;
        U32 SkillRecordBaseCooldown;
        U32 SkillInnerMaskOffset;
        U32 SkillNodeTargetType;
        U32 SkillNodeBaseCooldown;

        // MainUI Buff Vector offsets
        U32 MainUIBuffVectorFirst;
        U32 MainUIBuffVectorLast;

        // Quest offsets
        U32 QuestTextRecordStride;
        U32 QuestTextItemTypeSlot1;
        U32 QuestTextMobHunt1;
        U32 QuestTextMobHunt1Count;
        U32 QuestTextMobHunt2;
        U32 QuestTextMobHunt2Count;
        U32 QuestTextEndNpcType;
        U32 QuestTextEndNpcId;
        U32 NpcFileRecordStride;

        // Camera offsets
        U32 CameraSecondaryEyeOffset;

        // AutoLogin offsets
        U32 LoginCandidateSigOffset;
        U32 LoginDefocusOffset;
        U32 SelectServerOffset;
        U32 SelectServerIndexOffset1;
        U32 SelectServerIndexOffset2;
        U32 SelectServerActiveFlag;
        U32 NetworkUserBuffer;
        U32 NetworkPassBuffer;
        U32 NetworkServerCount;
        U32 NetworkCharStride;
        U32 CharSelectSlotBase;
        U32 CharSelectSlotBtnBase;
        U32 CharSelectSlotBtnStride;
        U32 CharSelectStartGameFlag;

        static bool Initialize();
    };

    extern GameOffsets Offsets;
}
