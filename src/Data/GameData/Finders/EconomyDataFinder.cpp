/**
 * @file EconomyDataFinder.cpp
 * @brief Implementation of pattern searcher for economy bank transaction functions.
 *
 * Discovers the entry points of ProcessBankDeposit, ProcessBankWithdrawal and
 * TryProcessTransaction by string anchors (verified against Ghidra 1.61).
 */

#include "SPF/Data/GameData/Finders/EconomyDataFinder.hpp"

#include "SPF/Data/GameData/EconomyService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include <cstdint>

namespace SPF::Data::GameData::Finders {

using namespace SPF::Utils;

bool EconomyDataFinder::TryFindOffsets(EconomyService& owner) {
  FinderLog log(GetName());

  // ── Phase 1: ProcessBankDeposit ──
  /*
   * FindFunctionByString with findStart=true returns function start.
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ProcessBankDeposit[140839910]) ---/
   * 140839910 40 55 PUSH RBP
   */
  {
    auto phase = log.MakePhase("ProcessBankDeposit");

    uintptr_t pfnDeposit = PatternFinder::FindFunctionByString("Incorrect bank deposit call", true);
    if (phase.Step(pfnDeposit, "ProcessBankDeposit", "FN")) {
      owner.SetProcessBankDepositAddr(pfnDeposit);

      // bank → balance offset
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ProcessBankDeposit[140839910]) ---/
      // * 140839962  48 8B 49 10                   MOV RCX,qword ptr [RCX + 0x10]
      uintptr_t addrMovBal = PatternFinder::Find(pfnDeposit, 128, "[MOV r64, [r64+off8]]");
      if (addrMovBal) {
        int balLen = 0;
        int32_t bankToBalance = PatternFinder::ReadInstructionDisp(addrMovBal, balLen);
        if (phase.StepOffset(bankToBalance, "bank→balance", "ECON")) {
          owner.SetBankToBalanceOffset(bankToBalance);
        }

        // bank → debt flag offset
        // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ProcessBankDeposit[140839910]) ---/
        // * 140839969  80 7B 6C 00                   CMP byte ptr [RBX + 0x6c],0x0
        uintptr_t addrCmpDebt = PatternFinder::Find(addrMovBal, 32, "[CMP byte ptr [r64+off8], imm8]");
        if (addrCmpDebt) {
          int debtLen = 0;
          int32_t bankDebtFlag = PatternFinder::ReadInstructionDisp(addrCmpDebt, debtLen);
          if (phase.StepOffset(bankDebtFlag, "bank→debt flag", "ECON")) {
            owner.SetBankDebtFlagOffset(bankDebtFlag);
          }
        } else {
          phase.StepOffset(0, "bank→debt flag", "ECON");
        }
      } else {
        phase.StepOffset(0, "bank→balance", "ECON");
      }
    }
  }

  // ── Phase 2: ProcessBankWithdrawal ──
  /*
   * FindFunctionByString with findStart=true returns function start.
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ProcessBankWithdrawal[140839ac0]) ---/
   * 140839ac0 40 55 PUSH RBP
   */
  {
    auto phase = log.MakePhase("ProcessBankWithdrawal");

    uintptr_t pfnWithdraw = PatternFinder::FindFunctionByString("Incorrect bank withdraw call", true);
    if (phase.Step(pfnWithdraw, "ProcessBankWithdrawal", "FN")) {
      owner.SetProcessBankWithdrawalAddr(pfnWithdraw);
    }
  }

  // ── Phase 3: TryProcessTransaction ──
  /*
   * FindFunctionByString with findStart=true returns function start.
   * contextSig "[CALL [r64+off8]] [MOV r64, [rip+off32]]" checked within ±64 bytes
   * of the string xref (both directions), then GetFunctionStart.
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(TryProcessTransaction[1407a6d40]) ---/
   * 1407a6d40 48 83 EC 58 SUB RSP,0x58
   * 1407a6d73 FF 50 08 CALL qword ptr [RAX + 0x8]
   * 1407a6d76 48 8B 0D 5B 0F F1 02 MOV RCX,qword ptr [0x1436b7cd8]
   * 1407a6d7d 48 8D 05 94 7E B0 01 LEA RAX,[0x1422aec18]
   * 1407a6d84 48 89 44 24 60 MOV qword ptr [RSP + 0x60],RAX
   */
  {
    auto phase = log.MakePhase("TryProcessTransaction");

    const char* contextSig = "[CALL [r64+off8]] [MOV r64, [rip+off32]]";
    uintptr_t pfnTxn = PatternFinder::FindFunctionByString("@@not_enough_money@@", true, contextSig, 64);
    if (phase.Step(pfnTxn, "TryProcessTransaction", "FN")) {
      owner.SetTryProcessTransactionAddr(pfnTxn);

      // economy → bank offset
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(TryProcessTransaction[1407a6d40]) ---/
      // * 1407a6d56  48 8B 41 10                   MOV RAX,qword ptr [RCX + 0x10]
      uintptr_t addrMovBank = PatternFinder::Find(pfnTxn, 32, "[MOV r64, [r64+off8]]");
      if (addrMovBank) {
        int bankLen = 0;
        int32_t economyToBank = PatternFinder::ReadInstructionDisp(addrMovBank, bankLen);
        if (phase.StepOffset(economyToBank, "economy→bank", "ECON")) {
          owner.SetEconomyToBankOffset(economyToBank);
        }
      } else {
        phase.StepOffset(0, "economy→bank", "ECON");
      }

      // economy → mail ctx offset
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(TryProcessTransaction[1407a6d40]) ---/
      // * 1407a6ded  48 8B 91 88 01 00 00          MOV RDX,qword ptr [RCX + 0x188]
      uintptr_t addrMovMail = PatternFinder::Find(pfnTxn, 184, "[MOV r64, [r64+off32]]");
      if (addrMovMail) {
        int mailLen = 0;
        int32_t economyToMailCtx = PatternFinder::ReadInstructionDisp(addrMovMail, mailLen);
        if (phase.StepOffset(economyToMailCtx, "economy→mail ctx", "ECON")) {
          owner.SetEconomyToMailCtxOffset(economyToMailCtx);
        }
      } else {
        phase.StepOffset(0, "economy→mail ctx", "ECON");
      }
    }
  }

  m_isReady = owner.GetProcessBankDepositAddr() != 0 && owner.GetProcessBankWithdrawalAddr() != 0 && owner.GetTryProcessTransactionAddr() != 0;
  return log.Finish(m_isReady);
}

}  // namespace SPF::Data::GameData::Finders
