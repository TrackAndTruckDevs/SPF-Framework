/**
 * @file MailDataFinder.cpp
 * @brief Implementation of pattern searcher for game mail system functions.
 *
 * Discovers CreateAndDeliverMail by string anchor (verified against Ghidra 1.61).
 * Session pattern: writes results directly to MailService via Set* methods.
 */

#include "SPF/Data/GameData/Finders/MailDataFinder.hpp"

#include "SPF/Data/GameData/MailService.hpp"
#include "SPF/Hooks/GameTools/PrismStringResolver.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include "fmt/format.h"

#include <cstdint>
#include <string>

namespace SPF::Data::GameData::Finders {
using namespace Utils;

bool MailDataFinder::TryFind(MailService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());

  // ── Phase 1: CreateAndDeliverMail + MailDefFactory ──
  uintptr_t mailDefFactoryAddr = 0;
  uintptr_t createAndDeliverMailAddr = 0;
  int32_t inboxArraySizeAbs = 0;
  {
    auto phase = log.MakePhase("CreateAndDeliverMail");

    /**
     * Anchor found WITHOUT going to the function start (findStart=false):
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(CreateAndDeliverMail[14083f790]) ---/
     * 14083f7eb  48 8D 0D 4E 28 A8 01          LEA RCX,[0x1422c2040]
     */
    uintptr_t leaSite = PatternFinder::FindFunctionByString("Mail '%s' does not exist!", false);
    if (phase.Step(leaSite, "anchor LEA site", "REF")) {
      /**
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(CreateAndDeliverMail[14083f790]) ---/
       * 14083f824  E8 57 E3 FF FF                CALL 0x14083db80        <- mail_def factory
       * 14083f829  48 8B D8                      MOV RBX,RAX
       */
      uintptr_t callSite = PatternFinder::Find(leaSite, 128, "[CALL rel32] [MOV r64, r64]");
      if (phase.Step(callSite, "factory CALL site", "RT")) {
        mailDefFactoryAddr = PatternFinder::GetRipAddress(callSite, 1, 5);
        owner.SetMailDefFactoryAddr(mailDefFactoryAddr);
        phase.Step(mailDefFactoryAddr, "MailDefFactory", "FN");
      }

      /**
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(CreateAndDeliverMail[14083f790]) ---/
       * 14083f790  40 53                         PUSH RBX
       */
      uintptr_t pfnDeliver = PatternFinder::GetFunctionStart(leaSite);
      if (phase.Step(pfnDeliver, "CreateAndDeliverMail", "FN")) {
        createAndDeliverMailAddr = pfnDeliver;
        owner.SetCreateAndDeliverMailAddr(createAndDeliverMailAddr);
      }
    }

    // MailDef factory layout extraction — chain of instruction-offset reads.
    if (mailDefFactoryAddr) {
      /**
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083db80[14083db80]) ---/
       * 14083dc1c  48 BB 00 00 00 00 01 00 00 00 MOV RBX,0x100000000
       */
      uintptr_t refcountIncSite = PatternFinder::Find(mailDefFactoryAddr, 256, "[MOV r64, imm64]");
      if (phase.Step(refcountIncSite, "refcount INC MOV", "RT")) {
        owner.SetCtrlRefcountInc(static_cast<uint64_t>(PatternFinder::ReadInt64(refcountIncSite + 2)));

        /**
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083db80[14083db80]) ---/
         * 14083dc13  4C 89 77 50                   MOV qword ptr [RDI + 0x50],R14
         * 14083dc17  E8 64 4A 8B FF                CALL 0x1400f2680
         */
        uintptr_t capStoreSite = PatternFinder::FindBackward(refcountIncSite, 32, "[MOV [r64+off8], r64] [CALL rel32]");
        if (phase.Step(capStoreSite, "array cap store", "RT")) {
          owner.SetArrayCapFieldOffset(static_cast<int32_t>(static_cast<uint8_t>(PatternFinder::ReadInt8(capStoreSite + 3))));
        }

        /**
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083db80[14083db80]) ---/
         * 14083dc2d  48 89 4F 58                   MOV qword ptr [RDI + 0x58],RCX
         */
        uintptr_t ctrlStoreSite = PatternFinder::Find(refcountIncSite, 32, "[MOV [r64+off8], r64]");
        if (phase.Step(ctrlStoreSite, "array ctrl store", "RT")) {
          owner.SetArrayCtrlFieldOffset(static_cast<int32_t>(static_cast<uint8_t>(PatternFinder::ReadInt8(ctrlStoreSite + 3))));
        }

        /**
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083db80[14083db80]) ---/
         * 14083dc26  48 8B 48 10                   MOV RCX,qword ptr [RAX + 0x10]
         */
        uintptr_t sharedCtrlSite = PatternFinder::Find(refcountIncSite, 32, "[MOV r64, [r64+off8]]");
        if (phase.Step(sharedCtrlSite, "shared ctrl inner MOV", "RT")) {
          owner.SetSharedCtrlInnerOff(static_cast<int32_t>(static_cast<uint8_t>(PatternFinder::ReadInt8(sharedCtrlSite + 3))));

          /**
           * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083db80[14083db80]) ---/
           * 14083dc31  F0 48 0F C1 41 58             XADD.LOCK qword ptr [RCX + 0x58],RAX
           */
          uintptr_t xaddSite = PatternFinder::Find(sharedCtrlSite, 64, "F0 48 0F C1 41");
          if (phase.Step(xaddSite, "refcount XADD", "RT")) {
            owner.SetCtrlRefcountOff(static_cast<int32_t>(static_cast<uint8_t>(PatternFinder::ReadInt8(xaddSite + 5))));

            /**
             * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083db80[14083db80]) ---/
             * 14083dc7e  66 44 89 B7 88 00 00 00       MOV word ptr [RDI + 0x88],R14W
             */
            uintptr_t readSite = PatternFinder::Find(xaddSite, 128, "66 44 89 B7");
            if (phase.Step(readSite, "read field MOV", "RT")) {
              owner.SetMailDefReadOffset(PatternFinder::ReadInt32(readSite + 4));

              /**
               * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083db80[14083db80]) ---/
               * 14083dc8d  44 89 B7 8C 00 00 00          MOV dword ptr [RDI + 0x8c],R14D
               * (search from readSite + 8)
               */
              uintptr_t sizeSite = PatternFinder::Find(readSite + 8, 32, "44 [MOV [r64+off32], r32]");
              if (phase.Step(sizeSite, "last field MOV", "RT")) {
                int32_t lastFieldOff = PatternFinder::ReadInt32(sizeSite + 3);
                owner.SetMailDefSize(lastFieldOff + 4);
              }
            }
          }
        }
      }
    }

    if (mailDefFactoryAddr) {
      /**
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(CreateAndDeliverMail[14083f790]) ---/
       * 14083f807  83 F8 04                      CMP EAX,0x4
       * (search from leaSite)
       */
      uintptr_t invalidIndexSite = PatternFinder::Find(leaSite, 32, "[CMP r32, imm8]");
      if (phase.Step(invalidIndexSite, "unit system invalid index", "RT")) {
        owner.SetUnitSystemInvalidIndex(PatternFinder::ReadInt8(invalidIndexSite + 2));

        /**
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(CreateAndDeliverMail[14083f790]) ---/
         * 14083f858  4C 8D 75 20                   LEA R14,[RBP + 0x20]
         * 14083f85c  48 8D 73 38                   LEA RSI,[RBX + 0x38]
         * 14083f860  4C 3B F6                      CMP R14,RSI
         */
        uintptr_t keysSite = PatternFinder::Find(invalidIndexSite + 4, 128, "[LEA r64, [r64+off8]] [LEA r64, [r64+off8]] [CMP r64, r64]");
        if (phase.Step(keysSite, "request keys offset", "RT")) {
          owner.SetRequestKeysOffset(PatternFinder::ReadInt8(keysSite + 3));

          /**
           * /--- Ghidra:(amtrucks_1_61.exe) Fun:(CreateAndDeliverMail[14083f790]) ---/
           * 14083f878  48 83 C5 48                   ADD RBP,0x48
           */
          uintptr_t valuesSite = PatternFinder::Find(keysSite + 8, 64, "[ADD r64, imm8]");
          if (phase.Step(valuesSite, "request values offset", "RT")) {
            owner.SetRequestValuesOffset(PatternFinder::ReadInt8(valuesSite + 3));

            /**
             * /--- Ghidra:(amtrucks_1_61.exe) Fun:(CreateAndDeliverMail[14083f790]) ---/
             * 14083f8a2  4D 8B 45 20                   MOV R8,qword ptr [R13 + 0x20]
             */
            uintptr_t sizeSite = PatternFinder::Find(valuesSite + 4, 64, "[MOV r64, [r64+off8]]");
            if (phase.Step(sizeSite, "inbox size abs offset", "RT")) {
              inboxArraySizeAbs = PatternFinder::ReadInt8(sizeSite + 3);

              /**
               * /--- Ghidra:(amtrucks_1_61.exe) Fun:(CreateAndDeliverMail[14083f790]) ---/
               * 14083f948  83 7F 14 00                   CMP dword ptr [RDI + 0x14],0x0
               */
              uintptr_t categorySite = PatternFinder::Find(sizeSite + 4, 256, "83 7F ? 00");
              if (phase.Step(categorySite, "mail def category offset", "RT")) {
                owner.SetMailDefCategoryOffset(PatternFinder::ReadInt8(categorySite + 2));
              }
            }
          }
        }
      }
    }
  }

  // ── Phase 2: Compose — build from/subject/body for an existing mail ──
  {
    auto phase = log.MakePhase("Compose");

    /**
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_141258cd0) ---/
     * Anchor string: "[mail_hdl] Unable to find mail text %s"
     * FindFunctionByString(anchor, true) -> function start (CALL rel32 decodes
     * to the function start directly, not to the LEA site inside it).
     */
    uintptr_t compose = PatternFinder::FindFunctionByString("[mail_hdl] Unable to find mail text %s", true);
    if (phase.Step(compose, "Compose function", "FN")) {
      owner.SetComposeAddr(compose);

      /**
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_141258cd0[141258cd0]) ---/
       * 141258ea2  48 C1 E6 05                   SHL RSI,0x5
       */
      uintptr_t prismSizeShl = PatternFinder::Find(compose, 500, "[SHL r64, imm8]");
      if (phase.Step(prismSizeShl, "temp prism size SHL", "RT")) {
        owner.SetTempPrismSize(1 << PatternFinder::ReadInt8(prismSizeShl + 3));
        /**
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_141258cd0[141258cd0]) ---/
         * 141258ead  48 8B 50 08                   MOV RDX,qword ptr [RAX + 0x8]
         */
        uintptr_t dataSite = PatternFinder::Find(prismSizeShl, 32, "[MOV r64, [r64+off8]]");
        if (phase.Step(dataSite, "array data MOV", "RT")) {
          owner.SetArrayDataOffset(static_cast<int32_t>(static_cast<uint8_t>(PatternFinder::ReadInt8(dataSite + 3))));
        }
      }
    }
  }

  // ── Phase 3: Delete-by-name — remove all mails whose name matches ──
  uintptr_t deleteByNameAddr = 0;
  {
    auto phase = log.MakePhase("Delete By Name");

    /**
     * /--- Ghidra:(amtrucks_1_61.exe) Call site in DeliverOvsNewRoutesMail ---/
     * 14084d093  e8 d8 2a ff ff   CALL FUN_14083fb70          <- mail delete by name
     * 14084d098  48 8d 55 d8      LEA RDX,[RBP + -0x28]       <- 2nd param (char**)
     *
     * Start from PrismStringResolver's etalon anchor (the same string LEA site
     * DeliverOvsNewRoutesMail uses: "mailtext.ovs.new_routes"), search forward
     * [CALL rel32] [LEA r64, [r64+off8]] within 64 bytes, resolve the CALL target.
     */
    uintptr_t anchor = Hooks::GameTools::PrismStringResolver::GetInstance().GetEtalonAnchorAddr();
    if (phase.Step(anchor, "etalon anchor (DeliverOvsNewRoutesMail string LEA)", "REF")) {
      uintptr_t callSite = PatternFinder::Find(anchor, 64, "[CALL rel32] [LEA r64, [r64+off8]]");
      if (phase.Step(callSite, "delete CALL site", "RT")) {
        deleteByNameAddr = PatternFinder::GetRipAddress(callSite, 1, 5);
        phase.Step(deleteByNameAddr, "DeleteByName", "FN");
      }
    }
  }

  // ── Phase 4: Remove-at-index — erase a single inbox/pending entry ──
  if (deleteByNameAddr) {
    auto phase = log.MakePhase("Remove At Index");

    /**
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083fb70) ---/
     * 14083fc0b  e8 40 10 00 00   CALL FUN_140840c50          <- remove-at-index
     * 14083fc10  4c 8b 53 18      MOV R10,[RBX + 0x18]        <- name buffer reload
     *
     * Search from the start of DeleteByName: [CALL rel32] [MOV r64, [r64+off8]]
     * within 256 bytes, resolve the CALL target.
     */
    uintptr_t callSite = PatternFinder::Find(deleteByNameAddr, 256, "[CALL rel32] [MOV r64, [r64+off8]]");
    uintptr_t removeAtIndexAddr = 0;
    if (phase.Step(callSite, "remove CALL site", "RT")) {
      removeAtIndexAddr = PatternFinder::GetRipAddress(callSite, 1, 5);
      owner.SetRemoveAtIndexAddr(removeAtIndexAddr);
      phase.Step(removeAtIndexAddr, "RemoveAtIndex", "FN");
    }
  }

  // ── Phase 5: LoadLocale — load /locale/<lang>/mails.sii and register mail text ──
  {
    auto phase = log.MakePhase("LoadLocale");

    /**
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083e740[14083e740]) ---/
     * 14083e762  48 8D 0D 3F AD 5A 00          LEA RCX,[0x142d894a8]   <- PTR_PTR_142d894a8 (unit array object)
     * 14083e871  48 8D 15 A0 C5 5D 01          LEA RDX,[0x141e1ae18]
     * FindFunctionByString(anchor, true) -> function start (FUN_14083e740).
     */
    uintptr_t loadLocale = PatternFinder::FindFunctionByString("/locale/%s/mails.sii", true);
    if (phase.Step(loadLocale, "LoadLocale function", "FN")) {
      owner.SetLoadLocaleAddr(loadLocale);
    }

    // PTR_PTR_142d894a8 — unit array object: [LEA r64, [rip+off32]] [MOV [r64+off8], r64].
    uintptr_t unitArrayObjectAddr = 0;
    if (loadLocale) {
      uintptr_t objLea = PatternFinder::Find(loadLocale, 64, "[LEA r64, [rip+off32]] [MOV [r64+off8], r64]");
      if (phase.Step(objLea, "unit array object LEA", "REF")) {
        unitArrayObjectAddr = PatternFinder::GetRipAddress(objLea, 3, 7);
        owner.SetUnitArrayObjectAddr(unitArrayObjectAddr);
        phase.Step(unitArrayObjectAddr, "unit array object", "DAT");
      }
    }

    /**
     * Collector FUN_140841c40: from the anchor LEA site (NOT the function start).
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083e740[14083e740]) ---/
     * 14083e923  E8 18 33 5B FF                CALL 0x140841c40        <- collect+filter
     * 14083e928  40 B7 01                      MOV DIL,0x1
     */
    uintptr_t collectorFn = 0;
    uintptr_t anchorLea = PatternFinder::FindFunctionByString("/locale/%s/mails.sii", false);
    if (phase.Step(anchorLea, "anchor LEA site", "REF")) {
      uintptr_t callSite = PatternFinder::Find(anchorLea, 256, "[CALL rel32] 40? [MOV r8, imm8]");
      if (phase.Step(callSite, "collector CALL site", "RT")) {
        collectorFn = PatternFinder::GetRipAddress(callSite, 1, 5);
        phase.Step(collectorFn, "MailUnitCollector", "FN");
      }
    }

    /**
     * Capacity CMP and the data/count chain inside the collector.
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140841c40[140841c40]) ---/
     * 140841d4a  48 3B 15 6F 77 54 02          CMP RDX,[0x142d894c0]   <- capacity
     * 140841d73  4C 8B 05 36 77 54 02          MOV R8,[0x142d894b0]    <- data
     * 140841d7a  48 8D 40 18                   LEA RAX,[RAX+0x18]
     * 140841d7e  48 89 15 33 77 54 02          MOV [0x142d894b8],RDX   <- count
     */
    if (collectorFn) {
      uintptr_t capCmp = PatternFinder::Find(collectorFn, 512, "48 3B 15");
      uintptr_t unitArrayCapacityAddr = 0;
      uintptr_t unitArrayDataAddr = 0;
      uintptr_t unitArrayCountAddr = 0;
      if (phase.Step(capCmp, "capacity CMP", "RT")) {
        unitArrayCapacityAddr = PatternFinder::GetRipAddress(capCmp, 3, 7);
        owner.SetUnitArrayCapacityAddr(unitArrayCapacityAddr);
        phase.Step(unitArrayCapacityAddr, "unit array capacity", "DAT");
      }
      if (capCmp) {
        uintptr_t chain = PatternFinder::Find(capCmp, 64, "[MOV r64, [rip+off32]] [LEA r64, [r64+off8]] [MOV [rip+off32], r64]");
        if (phase.Step(chain, "data/count chain", "RT")) {
          unitArrayDataAddr = PatternFinder::GetRipAddress(chain, 3, 7);
          unitArrayCountAddr = PatternFinder::GetRipAddress(chain + 11, 3, 7);
          owner.SetUnitArrayDataAddr(unitArrayDataAddr);
          owner.SetUnitArrayCountAddr(unitArrayCountAddr);
          phase.Step(unitArrayDataAddr, "unit array data", "DAT");
          phase.Step(unitArrayCountAddr, "unit array count", "DAT");
        }
      }
    }
  }

  // ── Phase 6: RegisterMailTextName — set+register a mail_text unit name ──
  {
    auto phase = log.MakePhase("RegisterMailTextName");

    /**
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14085b530[14085b530]) ---/
     * 14085b5d1  48 8D 05 98 B6 A6 01          LEA RAX,[0x1422c6c70] = "history.manager"
     * FindFunctionByString(anchor, false) -> LEA site (NOT the function start).
     * From the LEA site search [CALL rel32] within 32 bytes:
     * 14085b5e9  E8 92 05 8E FF                CALL 0x14013bb80
     * FUN_14013bb80(unit, nameString) — parses name, stores name_id at unit+0xC.
     */
    uintptr_t leaSite = PatternFinder::FindFunctionByString("history.manager", false);
    if (phase.Step(leaSite, "string LEA site", "REF")) {
      uintptr_t callSite = PatternFinder::Find(leaSite, 32, "[CALL rel32]");
      if (phase.Step(callSite, "register CALL site", "RT")) {
        uintptr_t registerNameAddr = PatternFinder::GetRipAddress(callSite, 1, 5);
        owner.SetRegisterNameAddr(registerNameAddr);
        phase.Step(registerNameAddr, "RegisterMailTextName", "FN");
      }
    }
  }

  // ── Phase 7: MailTextFactory — create an empty mail_text_u ──
  {
    auto phase = log.MakePhase("MailTextFactory");

    /**
     * Reflection chain (Ghidra verified):
     *   s_mail_text "mail_text" -> data ptr -> type descriptor 142d890a0
     *   -> data ptr PTR_PTR_141e1aee0 -> code xrefs inside the factory:
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083d740[14083d740]) ---/
     * 14083d740  40 53                         PUSH RBX
     * 14083d742  48 83 EC 20                   SUB RSP,0x20
     * 14083d746  48 8B D9                      MOV RBX,RCX
     * 14083d749  48 8D 0D 92 03 77 00          LEA RCX,[0x141e1aee0]
     *
     * The clone (FUN_14083d870) xrefs the same descriptor but its prolog has no
     * consecutive [SUB r64, imm8] [MOV r64, r64] pair at start+2 — filtered out.
     */
    uintptr_t factory = 0;
    uintptr_t strAddr = PatternFinder::FindString("mail_text");
    if (phase.Step(strAddr, "s_mail_text string", "STR")) {
      auto ptrs = PatternFinder::FindDataPointers(strAddr);
      for (uintptr_t ptr : ptrs) {
        auto descs = PatternFinder::FindDataPointers(ptr);
        for (uintptr_t desc : descs) {
          auto xrefs = PatternFinder::FindXrefs(desc);
          for (uintptr_t xref : xrefs) {
            uintptr_t fs = PatternFinder::GetFunctionStart(xref);
            if (!fs) continue;
            uintptr_t prolog = PatternFinder::Find(fs, 32, "[SUB r64, imm8] [MOV r64, r64]");
            if (prolog == fs + 2) {
              factory = fs;
              break;
            }
          }
          if (factory) break;
        }
        if (factory) break;
      }
    }
    if (phase.Step(factory, "MailTextFactory", "FN")) {
      owner.SetMailTextFactoryAddr(factory);
    }
  }

  // ── Phase 8: dynstring builder vtable (builder+0x00) ──
  {
    auto phase = log.MakePhase("DynStringVtable");

    /**
     * PTR_FUN_1421e6698 — dynstring builder vtable (builder+0x00):
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140f61570[140f61570]) ---/
     * 140f616e1  48 8D 15 54 C0 3F 01          LEA RDX,[0x14235d73c] = "%u mph"
     * FindBackward from that LEA site, [LEA r64, [rip+off32]], window 64:
     * 140f616b5  48 8D 2D DC 4F 28 01          LEA RBP,[0x1421e6698]
     */
    uintptr_t mphLea = PatternFinder::FindFunctionByString("%u mph", false);
    if (phase.Step(mphLea, "%u mph LEA site", "REF")) {
      // FindBackward can match the start site itself — the "%u mph" site is an
      // LEA too and its target is the format string, NOT the vtable. A real LEA
      // is 7 bytes, so start the backward search 7 bytes below the site to skip
      // it and land on the previous [LEA r64,[rip+off32]] (the vtable load).
      uintptr_t vtableLea = PatternFinder::FindBackward(mphLea - 7, 64, "[LEA r64, [rip+off32]]");
      if (phase.Step(vtableLea, "dynstring vtable LEA site", "RT")) {
        uintptr_t dynStringVtableAddr = PatternFinder::GetRipAddress(vtableLea, 3, 7);
        phase.Step(dynStringVtableAddr, "dynstring vtable", "DAT");
      }
    }
  }

  // ── Phase 9: mail_ctrl Reflection Attributes ──
  int32_t mailCtrlInboxOffset = 0;
  {
    auto phase = log.MakePhase("Mail Ctrl Reflection Attributes");

    const char* kMailCtrlClass = "mail_ctrl";

    auto getAttr = [&phase](const char* className, const char* attrName) -> uintptr_t {
      uintptr_t off = PatternFinder::FindAttributeOffset(className, attrName);
      std::string desc = fmt::format("{}::{}", className, attrName);
      phase.StepOffset(static_cast<int32_t>(off), desc, "REF");
      return off;
    };

    uintptr_t inbox = getAttr(kMailCtrlClass, "inbox");
    if (inbox) {
      mailCtrlInboxOffset = static_cast<int32_t>(inbox);
      owner.SetMailCtrlInboxOffset(mailCtrlInboxOffset);
    }

    uintptr_t unread = getAttr(kMailCtrlClass, "unread_count");
    if (unread) owner.SetMailCtrlUnreadOffset(static_cast<int32_t>(unread));

    uintptr_t pending = getAttr(kMailCtrlClass, "pending_mails");
    if (pending) owner.SetMailCtrlPendingOffset(static_cast<int32_t>(pending));
  }

  // Derive array size offset: inboxArraySizeAbs (from instruction) minus inbox base (from reflection).
  if (inboxArraySizeAbs && mailCtrlInboxOffset) {
    owner.SetArraySizeOffset(inboxArraySizeAbs - mailCtrlInboxOffset);
  }

  // ── Phase 10: mail_def / mail_text Reflection Attributes ──
  {
    auto phase = log.MakePhase("Mail Def/Text Reflection Attributes");

    auto getAttr = [&phase](const char* className, const char* attrName) -> uintptr_t {
      uintptr_t off = PatternFinder::FindAttributeOffset(className, attrName);
      std::string desc = fmt::format("{}::{}", className, attrName);
      phase.StepOffset(static_cast<int32_t>(off), desc, "REF");
      return off;
    };

    const char* kMailDefClass = "mail_def";
    const char* kMailTextClass = "mail_text";

    uintptr_t name = getAttr(kMailDefClass, "mail_text_ref");
    if (name) owner.SetMailDefNameOffset(static_cast<int32_t>(name));

    uintptr_t paramKeys = getAttr(kMailDefClass, "param_keys");
    if (paramKeys) owner.SetMailDefParamKeysOffset(static_cast<int32_t>(paramKeys));

    uintptr_t paramValues = getAttr(kMailDefClass, "param_values");
    if (paramValues) owner.SetMailDefParamValuesOffset(static_cast<int32_t>(paramValues));

    uintptr_t from = getAttr(kMailTextClass, "from");
    if (from) owner.SetMailTextFromOffset(static_cast<int32_t>(from));

    uintptr_t subject = getAttr(kMailTextClass, "subject");
    if (subject) owner.SetMailTextSubjectOffset(static_cast<int32_t>(subject));

    uintptr_t body = getAttr(kMailTextClass, "body");
    if (body) owner.SetMailTextBodyOffset(static_cast<int32_t>(body));
  }

  m_isReady = createAndDeliverMailAddr != 0;
  owner.SetInitialized(m_isReady);
  return log.Finish(m_isReady);
}

}  // namespace SPF::Data::GameData::Finders
