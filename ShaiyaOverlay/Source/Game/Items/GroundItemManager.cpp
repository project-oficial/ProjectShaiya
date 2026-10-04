#include "GroundItemManager.h"
#include "Core/Memory.h"
#include "Core/StringUtils.h"
#include "Game/GameOffsets.h"
#include "Game/Entities/EntityManager.h"

namespace ShaiyaOverlay
{
    FixedList<GroundItem, 128> GroundItemManager::Items;

    const char* GroundItemManager::GetCategoryName(U8 Type)
    {
        switch (Type)
        {
        case 1:  return "1H Sword";
        case 2:  return "2H Sword";
        case 3:  return "1H Axe";
        case 4:  return "2H Axe";
        case 5:  return "Dual Weapon";
        case 6:  return "Spear";
        case 7:  return "1H Blunt";
        case 8:  return "2H Blunt";
        case 9:  return "Reverse Dagger";
        case 10: return "Bow";
        case 11: return "Crossbow";
        case 12: return "Staff";
        case 13: return "Javelin";
        case 14: return "Knuckle";
        case 15: return "Shield";
        case 16: return "Helm";
        case 17: return "Upper Armor";
        case 18: return "Lower Armor";
        case 19: return "Shield";
        case 20: return "Gloves";
        case 21: return "Boots";
        case 22: return "Ring";
        case 23: return "Necklace";
        case 24: return "Bracelet";
        case 25: return "Potion";
        case 30: return "Lapis";
        case 44: return "Gold";
        default: return "Misc";
        }
    }

    bool GroundItemManager::ResolveItemName(U8 Type, U8 TypeId, char* OutName, U32 MaxLen)
    {
        if (!OutName || MaxLen == 0)
            return false;

        if (Type == 44)
        {
            StringUtils::Copy(OutName, "Gold", MaxLen);
            return true;
        }

        if (!Offsets.ItemDb)
        {
            StringUtils::Format(OutName, MaxLen, "Item [%u-%u]", Type, TypeId);
            return false;
        }

        U64 HeadNode = 0;
        if (!Memory::ReadSafe(Offsets.ItemDb + 0x10, &HeadNode) || !HeadNode)
        {
            StringUtils::Format(OutName, MaxLen, "Item [%u-%u]", Type, TypeId);
            return false;
        }

        U64 CurrTypeNode = 0;
        if (!Memory::ReadSafe(HeadNode, &CurrTypeNode) || !CurrTypeNode)
        {
            StringUtils::Format(OutName, MaxLen, "Item [%u-%u]", Type, TypeId);
            return false;
        }

        U32 TypeWalk = 0;
        while (CurrTypeNode && CurrTypeNode != HeadNode && TypeWalk < 128)
        {
            U32 NodeKey = 0;
            Memory::ReadSafe(CurrTypeNode + 16, &NodeKey);

            if (NodeKey == Type)
            {
                U64 InnerHead = 0;
                Memory::ReadSafe(CurrTypeNode + 32 + 0x10, &InnerHead);

                if (InnerHead)
                {
                    U64 CurrItemNode = 0;
                    Memory::ReadSafe(InnerHead, &CurrItemNode);

                    U32 ItemWalk = 0;
                    while (CurrItemNode && CurrItemNode != InnerHead && ItemWalk < 300)
                    {
                        U32 ItemKey = 0;
                        Memory::ReadSafe(CurrItemNode + 16, &ItemKey);

                        if (ItemKey == TypeId)
                        {
                            U64 RecPtr = 0;
                            Memory::ReadSafe(CurrItemNode + 24, &RecPtr);

                            if (RecPtr)
                            {
                                U64 Cap = 0;
                                Memory::ReadSafe(RecPtr + 24, &Cap);
                                U64 StrSource = RecPtr;
                                if (Cap > 15)
                                {
                                    Memory::ReadSafe(RecPtr, &StrSource);
                                }

                                if (StrSource)
                                {
                                    char TempName[64] = { 0 };
                                    if (Memory::ReadBytesSafe(StrSource, TempName, sizeof(TempName) - 1))
                                    {
                                        TempName[sizeof(TempName) - 1] = '\0';
                                        if (TempName[0] != '\0')
                                        {
                                            StringUtils::Copy(OutName, TempName, MaxLen);
                                            return true;
                                        }
                                    }
                                }
                            }
                            break;
                        }

                        Memory::ReadSafe(CurrItemNode, &CurrItemNode);
                        ++ItemWalk;
                    }
                }
                break;
            }

            Memory::ReadSafe(CurrTypeNode, &CurrTypeNode);
            ++TypeWalk;
        }

        StringUtils::Format(OutName, MaxLen, "%s [%u-%u]", GetCategoryName(Type), Type, TypeId);
        return false;
    }

    void GroundItemManager::Update()
    {
        Items.Clear();

        if (!Offsets.WorldManager)
            return;

        U64 ItemMapAddr = Offsets.WorldManager + Offsets.ItemMapOffset;
        U64 HeadNode = 0;
        if (!Memory::ReadSafe(ItemMapAddr + 0x10, &HeadNode) || !HeadNode)
            return;

        U64 CurrentNode = 0;
        if (!Memory::ReadSafe(HeadNode, &CurrentNode) || !CurrentNode)
            return;

        const PlayerData& Player = EntityManager::GetLocalPlayer();
        U32 Iterations = 0;
        constexpr U32 MaxItemsScan = 128;

        while (CurrentNode && CurrentNode != HeadNode && Iterations < MaxItemsScan)
        {
            U64 NextNode = 0;
            Memory::ReadSafe(CurrentNode, &NextNode);

            U64 ItemPtr = 0;
            Memory::ReadSafe(CurrentNode + 0x18, &ItemPtr);

            if (ItemPtr)
            {
                GroundItem Item = { 0 };
                Memory::ReadSafe(ItemPtr + Offsets.ItemWorldId, &Item.WorldId);
                Memory::ReadSafe(ItemPtr + Offsets.ItemOwnerId, &Item.OwnerId);
                Memory::ReadSafe(ItemPtr + Offsets.ItemDespawnTime, &Item.DespawnTime);

                Memory::ReadSafe(ItemPtr + Offsets.ItemPosX, &Item.Position.X);
                Memory::ReadSafe(ItemPtr + Offsets.ItemPosY, &Item.Position.Y);
                Memory::ReadSafe(ItemPtr + Offsets.ItemPosZ, &Item.Position.Z);

                Memory::ReadSafe(ItemPtr + Offsets.ItemType, &Item.Type);
                Memory::ReadSafe(ItemPtr + Offsets.ItemTypeId, &Item.TypeId);
                Memory::ReadSafe(ItemPtr + Offsets.ItemCount, &Item.Count);
                Memory::ReadSafe(ItemPtr + Offsets.ItemQuality, &Item.Quality);

                if (Player.Valid)
                    Item.Distance = Item.Position.DistanceTo(Player.Position);
                else
                    Item.Distance = 0.0f;

                StringUtils::Copy(Item.Category, GetCategoryName(Item.Type), sizeof(Item.Category));

                if (Item.Type == 44)
                {
                    StringUtils::Format(Item.Name, sizeof(Item.Name), "Gold (%u)", Item.Count);
                }
                else
                {
                    ResolveItemName(Item.Type, Item.TypeId, Item.Name, sizeof(Item.Name));
                }

                Items.Add(Item);
            }

            CurrentNode = NextNode;
            ++Iterations;
        }
    }
}
