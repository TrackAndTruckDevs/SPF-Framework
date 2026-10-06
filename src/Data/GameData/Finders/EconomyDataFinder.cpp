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

#include "fmt/format.h"

#include <cstdint>
#include <string>

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

  // ── Phase 4: SII reflection attributes (bank / bank_data / bank_loan_data / insurance_data) ──
  // Class names follow the reflection owner chains: runtime bank object class "bank"
  // (file /def/bank_data.sii), config class "bank_data", plus bank_loan_data and insurance_data.
  {
    auto phase = log.MakePhase("SII Reflection Attributes");

    auto getAttr = [&phase](const char* className, const char* attrName) -> uintptr_t {
      uintptr_t off = PatternFinder::FindAttributeOffset(className, attrName);
      std::string desc = fmt::format("{}::{}", className, attrName);
      phase.StepOffset(static_cast<int32_t>(off), desc, "REF");
      return off;
    };

    uintptr_t off = 0;
    if ((off = getAttr("bank_data", "min_after_loan_repay"))) owner.SetAttrMinAfterLoanRepay(static_cast<int32_t>(off));
    if ((off = getAttr("bank_data", "loan_offers"))) owner.SetAttrLoanOffers(static_cast<int32_t>(off));
    if ((off = getAttr("bank_data", "overdraft_duration"))) owner.SetAttrOverdraftDuration(static_cast<int32_t>(off));
    if ((off = getAttr("bank_data", "overdraft_warn_day"))) owner.SetAttrOverdraftWarnDay(static_cast<int32_t>(off));
    if ((off = getAttr("bank_data", "insurance_data"))) owner.SetAttrInsuranceData(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "money_account"))) owner.SetAttrMoneyAccount(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "coinsurance_fixed"))) owner.SetAttrCoinsuranceFixed(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "coinsurance_ratio"))) owner.SetAttrCoinsuranceRatio(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "accident_severity"))) owner.SetAttrAccidentSeverity(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "loans"))) owner.SetAttrLoans(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "app_enabled"))) owner.SetAttrAppEnabled(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "loan_limit"))) owner.SetAttrLoanLimit(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "payment_timer"))) owner.SetAttrPaymentTimer(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "overdraft"))) owner.SetAttrOverdraft(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "overdraft_timer"))) owner.SetAttrOverdraftTimer(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "overdraft_warn_count"))) owner.SetAttrOverdraftWarnCount(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "sell_players_truck_later"))) owner.SetAttrSellPlayersTruckLater(static_cast<int32_t>(off));
    if ((off = getAttr("bank", "sell_players_trailer_later"))) owner.SetAttrSellPlayersTrailerLater(static_cast<int32_t>(off));

    if ((off = getAttr("bank_loan_data", "amount"))) owner.SetAttrLoanAmount(static_cast<int32_t>(off));
    if ((off = getAttr("bank_loan_data", "interest_rate"))) owner.SetAttrLoanInterestRate(static_cast<int32_t>(off));
    if ((off = getAttr("bank_loan_data", "duration"))) owner.SetAttrLoanDuration(static_cast<int32_t>(off));

    if ((off = getAttr("insurance_data", "coinsurance_fixed_base"))) owner.SetAttrCoinsuranceFixedBase(static_cast<int32_t>(off));
    if ((off = getAttr("insurance_data", "coinsurance_fixed_min"))) owner.SetAttrCoinsuranceFixedMin(static_cast<int32_t>(off));
    if ((off = getAttr("insurance_data", "coinsurance_fixed_max"))) owner.SetAttrCoinsuranceFixedMax(static_cast<int32_t>(off));
    if ((off = getAttr("insurance_data", "coinsurance_fixed_inc"))) owner.SetAttrCoinsuranceFixedInc(static_cast<int32_t>(off));
    if ((off = getAttr("insurance_data", "coinsurance_fixed_dec"))) owner.SetAttrCoinsuranceFixedDec(static_cast<int32_t>(off));
    if ((off = getAttr("insurance_data", "coinsurance_ratio_base"))) owner.SetAttrCoinsuranceRatioBase(static_cast<int32_t>(off));
    if ((off = getAttr("insurance_data", "coinsurance_ratio_min"))) owner.SetAttrCoinsuranceRatioMin(static_cast<int32_t>(off));
    if ((off = getAttr("insurance_data", "coinsurance_ratio_max"))) owner.SetAttrCoinsuranceRatioMax(static_cast<int32_t>(off));
    if ((off = getAttr("insurance_data", "coinsurance_ratio_inc"))) owner.SetAttrCoinsuranceRatioInc(static_cast<int32_t>(off));
    if ((off = getAttr("insurance_data", "coinsurance_ratio_dec"))) owner.SetAttrCoinsuranceRatioDec(static_cast<int32_t>(off));
  }

  // ── Phase 5: ConfirmBankDialogAction — bank loan struct offsets ──
  /*
   * Function located by prologue template, all offsets extracted by chained
   * Find + ReadInstructionDisp from the anchors below (verified Ghidra 1.61).
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
   * 141255f50  48 89 5C 24 10                MOV qword ptr [RSP + 0x10],RBX
   * 141255f55  48 89 6C 24 18                MOV qword ptr [RSP + 0x18],RBP
   * 141255f5a  48 89 74 24 20                MOV qword ptr [RSP + 0x20],RSI
   * 141255f5f  57                            PUSH RDI
   * 141255f60  41 56                         PUSH R14
   * 141255f62  41 57                         PUSH R15
   * 141255f64  48 83 EC 50                   SUB RSP,0x50
   * 141255f68  48 8B 1D 91 8D DF 01          MOV RBX,qword ptr [0x14304ed00]
   * 141255f6f  8D 42 9C                      LEA EAX,[RDX + -0x64]
   */
  {
    auto phase = log.MakePhase("ConfirmBankDialogAction");

    uintptr_t pfnConfirm = PatternFinder::Find("[MOV [r64+off8], r64] [MOV [r64+off8], r64] [MOV [r64+off8], r64] [PUSH r64] [PUSH R8-R15] [PUSH R8-R15] [SUB r64, imm8] [MOV r64, [rip+off32]] [LEA r32, [r64+off8]]");
    if (phase.Step(pfnConfirm, "ConfirmBankDialogAction", "FN")) {
      owner.SetConfirmBankDialogAddr(pfnConfirm);

      // bank → offers container (+0x60)
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141255f82  49 8B 4E 60                   MOV RCX,qword ptr [R14 + 0x60]
      // * 141255f86  48 83 C1 18                   ADD RCX,0x18
      uintptr_t addrOffers = PatternFinder::Find(pfnConfirm, 64, "[MOV r64, [r64+off8]] [ADD r64, imm8]");
      int lenOffers = 0;
      int32_t bankToOffers = addrOffers ? PatternFinder::ReadInstructionDisp(addrOffers, lenOffers) : 0;
      if (phase.StepOffset(bankToOffers, "bank→offers", "ECON")) {
        owner.SetBankToOffersOffset(bankToOffers);
      }

      // bank → can loan guard (+0x50)
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 14125602c  41 80 7E 50 00                CMP byte ptr [R14 + 0x50],0x0
      uintptr_t addrGuard = addrOffers ? PatternFinder::Find(addrOffers, 256, "41? [CMP byte ptr [r64+off8], imm8]") : 0;
      int lenGuard = 0;
      int32_t bankCanLoan = addrGuard ? PatternFinder::ReadInstructionDisp(addrGuard, lenGuard) : 0;
      if (phase.StepOffset(bankCanLoan, "bank→can loan", "ECON")) {
        owner.SetBankCanLoanOffset(bankCanLoan);
      }

      // bank → active loans container (+0x28)
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141256063  49 8D 4E 28                   LEA RCX,[R14 + 0x28]
      // * 141256067  E8 A4 E8 E9 FE                CALL 0x1400f4910
      uintptr_t addrLoans = addrGuard ? PatternFinder::Find(addrGuard, 64, "[LEA r64, [r64+off8]]") : 0;
      int lenLoans = 0;
      int32_t bankToLoans = addrLoans ? PatternFinder::ReadInstructionDisp(addrLoans, lenLoans) : 0;
      if (phase.StepOffset(bankToLoans, "bank→loans", "ECON")) {
        owner.SetBankToLoansOffset(bankToLoans);
      }

      // active loans count (+0x38) / ptr (+0x30)
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141256070  49 8B 7E 38                   MOV RDI,qword ptr [R14 + 0x38]
      // * 141256074  4D 8B 76 30                   MOV R14,qword ptr [R14 + 0x30]
      uintptr_t addrLoanArr = addrLoans ? PatternFinder::Find(addrLoans, 32, "[MOV r64, [r64+off8]] [MOV r64, [r64+off8]]") : 0;
      int lenLoanCount = 0;
      int32_t loansCount = addrLoanArr ? PatternFinder::ReadInstructionDisp(addrLoanArr, lenLoanCount) : 0;
      if (phase.StepOffset(loansCount, "bank→loans count", "ECON")) {
        owner.SetLoansCountOffset(loansCount);
      }
      int lenLoanPtr = 0;
      int32_t loansPtr = (addrLoanArr && lenLoanCount) ? PatternFinder::ReadInstructionDisp(addrLoanArr + lenLoanCount, lenLoanPtr) : 0;
      if (phase.StepOffset(loansPtr, "bank→loans ptr", "ECON")) {
        owner.SetLoansPtrOffset(loansPtr);
      }

      // offer proposal fields: weeks (+0x18), rate (+0x14), amount (+0x10)
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141255fb8  44 8B 61 18                   MOV R12D,dword ptr [RCX + 0x18]
      // * 141255fbc  F3 0F 10 71 14                MOVSS XMM6,dword ptr [RCX + 0x14]
      // * 141255fc1  8B 79 10                      MOV EDI,dword ptr [RCX + 0x10]
      uintptr_t addrOfferFields = addrOffers ? PatternFinder::Find(addrOffers, 128, "44 [MOV r32, [r64+off8]] [MOVSS xmm, [r64+off8]] [MOV r32, [r64+off8]]") : 0;
      int lenOfferWeeks = 0;
      int32_t offerWeeks = addrOfferFields ? PatternFinder::ReadInstructionDisp(addrOfferFields, lenOfferWeeks) : 0;
      if (phase.StepOffset(offerWeeks, "offer→weeks", "ECON")) {
        owner.SetLoanOfferWeeksOffset(offerWeeks);
      }
      // MOVSS (F3 0F 10 71 14): disp8 sits at instruction+4, ReadInstructionDisp skips only REX
      uintptr_t addrOfferRate = (addrOfferFields && lenOfferWeeks) ? addrOfferFields + lenOfferWeeks : 0;
      int32_t offerRate = addrOfferRate ? PatternFinder::ReadInt8(addrOfferRate + 4) : 0;
      if (phase.StepOffset(offerRate, "offer→rate", "ECON")) {
        owner.SetLoanOfferRateOffset(offerRate);
      }
      uintptr_t addrOfferAmount = addrOfferRate ? PatternFinder::Find(addrOfferRate, 32, "[MOV r32, [r64+off8]]") : 0;
      int lenOfferAmount = 0;
      int32_t offerAmount = addrOfferAmount ? PatternFinder::ReadInstructionDisp(addrOfferAmount, lenOfferAmount) : 0;
      if (phase.StepOffset(offerAmount, "offer→amount", "ECON")) {
        owner.SetLoanOfferAmountOffset(offerAmount);
      }
      if (addrOfferFields) owner.SetAddrOfferFields(addrOfferFields);

      // active loan record fields: rate (+0x24), amount (+0x18), remaining (+0x10), weeks (+0x28), start day (+0x20)
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141256008  F3 0F 11 76 24                MOVSS dword ptr [RSI + 0x24],XMM6
      // * 14125600d  48 89 7E 18                   MOV qword ptr [RSI + 0x18],RDI
      // * 141256011  48 89 7E 10                   MOV qword ptr [RSI + 0x10],RDI
      // * 141256015  44 89 66 28                   MOV dword ptr [RSI + 0x28],R12D
      // * 141256019  89 6E 20                      MOV dword ptr [RSI + 0x20],EBP
      uintptr_t addrLoanFields = addrOfferFields ? PatternFinder::Find(addrOfferFields, 128, "[MOVSS [r64+off8], xmm] [MOV [r64+off8], r64] [MOV [r64+off8], r64] 44 [MOV [r64+off8], r32] [MOV [r64+off8], r32]") : 0;
      // MOVSS store (F3 0F 11 76 24): disp8 sits at instruction+4, ReadInstructionDisp skips only REX
      int32_t loanRate = addrLoanFields ? PatternFinder::ReadInt8(addrLoanFields + 4) : 0;
      if (phase.StepOffset(loanRate, "loan→rate", "ECON")) {
        owner.SetLoanRateOffset(loanRate);
      }
      uintptr_t addrLoanAmount = addrLoanFields ? PatternFinder::Find(addrLoanFields, 32, "[MOV [r64+off8], r64]") : 0;
      int lenLoanAmount = 0;
      int32_t loanAmount = addrLoanAmount ? PatternFinder::ReadInstructionDisp(addrLoanAmount, lenLoanAmount) : 0;
      if (phase.StepOffset(loanAmount, "loan→amount", "ECON")) {
        owner.SetLoanAmountOffset(loanAmount);
      }
      uintptr_t addrLoanRemaining = (addrLoanAmount && lenLoanAmount) ? addrLoanAmount + lenLoanAmount : 0;
      int lenLoanRemaining = 0;
      int32_t loanRemaining = addrLoanRemaining ? PatternFinder::ReadInstructionDisp(addrLoanRemaining, lenLoanRemaining) : 0;
      if (phase.StepOffset(loanRemaining, "loan→remaining", "ECON")) {
        owner.SetLoanRemainingOffset(loanRemaining);
      }
      uintptr_t addrLoanWeeks = (addrLoanRemaining && lenLoanRemaining) ? addrLoanRemaining + lenLoanRemaining : 0;
      int lenLoanWeeks = 0;
      int32_t loanWeeks = addrLoanWeeks ? PatternFinder::ReadInstructionDisp(addrLoanWeeks, lenLoanWeeks) : 0;
      if (phase.StepOffset(loanWeeks, "loan→weeks", "ECON")) {
        owner.SetLoanWeeksOffset(loanWeeks);
      }
      uintptr_t addrLoanStartDay = (addrLoanWeeks && lenLoanWeeks) ? addrLoanWeeks + lenLoanWeeks : 0;
      int lenLoanStartDay = 0;
      int32_t loanStartDay = addrLoanStartDay ? PatternFinder::ReadInstructionDisp(addrLoanStartDay, lenLoanStartDay) : 0;
      if (phase.StepOffset(loanStartDay, "loan→start day", "ECON")) {
        owner.SetLoanStartDayOffset(loanStartDay);
      }
    }
  }

  // ── Phase 6: OpenNewLoanConfirmDialog — loan offers container offsets ──
  /*
   * Function found by string anchor, findStart=true returns function start.
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(OpenNewLoanConfirmDialog[141255870]) ---/
   * 141255870  4C 8B DC                      MOV R11,RSP
   */
  {
    auto phase = log.MakePhase("OpenNewLoanConfirmDialog");

    uintptr_t pfnOpen = PatternFinder::FindFunctionByString("@@bank_nl_confirm_info@@", true);
    if (phase.Step(pfnOpen, "OpenNewLoanConfirmDialog", "FN")) {
      owner.SetOpenNewLoanConfirmAddr(pfnOpen);

      // offers container: base add (+0x18) / count (+0x10)
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(OpenNewLoanConfirmDialog[141255870]) ---/
      // * 14125589e  48 83 C1 18                   ADD RCX,0x18
      // * 1412558a2  4C 3B 79 10                   CMP R15,qword ptr [RCX + 0x10]
      uintptr_t addrAdd = PatternFinder::Find(pfnOpen, 64, "[ADD r64, imm8] [CMP r64, [r64+off8]]");
      int lenAdd = 0;
      int32_t offersBaseAdd = 0;
      if (addrAdd) {
        PatternFinder::ReadInstructionDisp(addrAdd, lenAdd);
        // ADD r64, imm8 (48 83 C1 18): no memory disp, imm8 sits right after the ModRM byte
        if (lenAdd) offersBaseAdd = PatternFinder::ReadInt8(addrAdd + lenAdd);
      }
      if (phase.StepOffset(offersBaseAdd, "offers→base add", "ECON")) {
        owner.SetOffersBaseAddOffset(offersBaseAdd);
      }

      uintptr_t addrOffersCount = addrAdd ? PatternFinder::Find(addrAdd, 64, "[CMP r64, [r64+off8]]") : 0;
      int lenOffersCount = 0;
      int32_t offersCount = addrOffersCount ? PatternFinder::ReadInstructionDisp(addrOffersCount, lenOffersCount) : 0;
      if (phase.StepOffset(offersCount, "offers→count", "ECON")) {
        owner.SetOffersCountOffset(offersCount);
      }

      // offers ptr relative to post-ADD base
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(OpenNewLoanConfirmDialog[141255870]) ---/
      // * 1412558ca  4C 8B 61 08                   MOV R12,qword ptr [RCX + 0x8]
      uintptr_t addrOffersPtr = addrAdd ? PatternFinder::Find(addrAdd, 64, "[MOV r64, [r64+off8]]") : 0;
      int lenOffersPtr = 0;
      int32_t offersPtr = addrOffersPtr ? PatternFinder::ReadInstructionDisp(addrOffersPtr, lenOffersPtr) : 0;
      if (phase.StepOffset(offersPtr, "offers→ptr", "ECON")) {
        owner.SetOffersPtrOffset(offersPtr);
      }
    }
  }

  // ── Phase 7: loan function entries — chained CALL extraction from ConfirmBankDialogAction ──
  {
    auto phase = log.MakePhase("Loan Function Entries");

    uintptr_t pfnConfirm = owner.GetConfirmBankDialogAddr();
    if (pfnConfirm) {
      // GetCreditLimit
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 14125603f  E8 2C 3C 5E FF                CALL 0x140839c70
      // * 141256044  48 3B C7                      CMP RAX,RDI
      uintptr_t addrCallLimit = PatternFinder::Find(pfnConfirm, 256, "[CALL rel32] [CMP r64, r64]");
      uintptr_t pfnGetCreditLimit = addrCallLimit ? PatternFinder::GetRipAddress(addrCallLimit, 1, 5) : 0;
      if (phase.Step(pfnGetCreditLimit, "GetCreditLimit", "FN")) {
        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(GetCreditLimit[140839c70]) ---/
         * 140839c70  48 89 5C 24 08                MOV qword ptr [RSP + 0x8],RBX
         */
        owner.SetGetCreditLimitAddr(pfnGetCreditLimit);
      }

      // AddLoan
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141256067  E8 A4 E8 E9 FE                CALL 0x1400f4910
      // * 14125606c  84 C0                         TEST AL,AL
      uintptr_t addrCallAdd = addrCallLimit ? PatternFinder::Find(addrCallLimit, 64, "[CALL rel32] [TEST r8, r8]") : 0;
      uintptr_t pfnAddLoan = addrCallAdd ? PatternFinder::GetRipAddress(addrCallAdd, 1, 5) : 0;
      if (phase.Step(pfnAddLoan, "AddLoan", "FN")) {
        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(AddLoan[1400f4910]) ---/
         * 1400f4910  48 89 5C 24 08                MOV qword ptr [RSP + 0x8],RBX
         */
        owner.SetAddLoanAddr(pfnAddLoan);
      }

      // CanRepayLoan — start at AddLoan call +5 to skip the self-match
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141256152  E8 B9 3B 5E FF                CALL 0x140839d10
      // * 141256157  84 C0                         TEST AL,AL
      uintptr_t addrCallRepay = addrCallAdd ? PatternFinder::Find(addrCallAdd + 5, 256, "[CALL rel32] [TEST r8, r8]") : 0;
      uintptr_t pfnCanRepayLoan = addrCallRepay ? PatternFinder::GetRipAddress(addrCallRepay, 1, 5) : 0;
      if (phase.Step(pfnCanRepayLoan, "CanRepayLoan", "FN")) {
        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(CanRepayLoan[140839d10]) ---/
         * 140839d10  48 83 EC 48                   SUB RSP,0x48
         */
        owner.SetCanRepayLoanAddr(pfnCanRepayLoan);
      }

      // RemoveLoan
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141256179  E8 52 6F 5E FF                CALL 0x14083d0d0
      // * 14125617e  49 8B CF                      MOV RCX,R15
      // * 141256181  E8 6A E4 FF FF                CALL 0x1412545f0
      uintptr_t addrCallRemove = addrCallRepay ? PatternFinder::Find(addrCallRepay, 64, "[CALL rel32] [MOV r64, r64] [CALL rel32]") : 0;
      uintptr_t pfnRemoveLoan = addrCallRemove ? PatternFinder::GetRipAddress(addrCallRemove, 1, 5) : 0;
      if (phase.Step(pfnRemoveLoan, "RemoveLoan", "FN")) {
        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(RemoveLoan[14083d0d0]) ---/
         * 14083d0d0  40 57                         PUSH RDI
         */
        owner.SetRemoveLoanAddr(pfnRemoveLoan);
      }

      // loan object pool free — from AddLoan call (141256067), forward
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 14125609e  E8 4D 71 EA FE                CALL 0x1400fd1f0
      // * 1412560a3  49 89 74 FE F8                MOV qword ptr [R14 + RDI*0x8 + -0x8],RSI
      uintptr_t addrFreeLoan = addrCallAdd ? PatternFinder::Find(addrCallAdd, 128, "[CALL rel32] [MOV [r64+off8], r64]") : 0;
      uintptr_t pfnFreeLoan = addrFreeLoan ? PatternFinder::GetRipAddress(addrFreeLoan, 1, 5) : 0;
      if (phase.Step(pfnFreeLoan, "loan object pool free", "FN")) {
        owner.SetFreeRefCountAddr(pfnFreeLoan);
      }
    }
  }

  // ── Phase 8: loan creation — factory + unit system table from ConfirmBankDialogAction ──
  {
    auto phase = log.MakePhase("Loan Create Entry");

    uintptr_t pfnConfirm = owner.GetConfirmBankDialogAddr();
    uintptr_t addrOfferFields = owner.GetAddrOfferFields();
    if (pfnConfirm && addrOfferFields) {
      // CreateBankLoan factory
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141255fe1  E8 0A 4E 86 FF                CALL 0x140abadf0
      // * 141255fe6  48 8B F0                      MOV RSI,RAX
      uintptr_t addrFactory = PatternFinder::Find(pfnConfirm, 256, "[CALL rel32] [MOV r64, r64]");
      uintptr_t pfnCreateLoan = addrFactory ? PatternFinder::GetRipAddress(addrFactory, 1, 5) : 0;
      if (phase.Step(pfnCreateLoan, "CreateBankLoan", "FN")) {
        owner.SetCreateLoanAddr(pfnCreateLoan);
      }

      // unit system index (DAT_14273b848) — backward from addrOfferFields
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141255fb2  8B 05 90 58 4E 01             MOV EAX,dword ptr [0x14273b848]
      uintptr_t addrUnitIndex = PatternFinder::FindBackward(addrOfferFields, 64, "[MOV r32, [rip+off32]]");
      uintptr_t unitIndexAddr = addrUnitIndex ? PatternFinder::GetRipAddress(addrUnitIndex, 2, 6) : 0;
      if (phase.Step(unitIndexAddr, "unit system index", "ECON")) {
        owner.SetUnitSystemIndexAddr(unitIndexAddr);
      }

      // unit system table (DAT_143687df8) — forward from addrOfferFields
      // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConfirmBankDialogAction[141255f50]) ---/
      // * 141255fcd  48 8D 0D 24 1E 43 02          LEA RCX,[0x143687df8]
      uintptr_t addrUnitTable = PatternFinder::Find(addrOfferFields, 64, "[LEA r64, [rip+off32]]");
      uintptr_t unitTableAddr = addrUnitTable ? PatternFinder::GetRipAddress(addrUnitTable, 3, 7) : 0;
      if (phase.Step(unitTableAddr, "unit system table", "ECON")) {
        owner.SetUnitSystemTableAddr(unitTableAddr);
      }
    }
  }

  m_isReady = owner.GetProcessBankDepositAddr() != 0 && owner.GetProcessBankWithdrawalAddr() != 0 && owner.GetTryProcessTransactionAddr() != 0;
  bool result = log.Finish(m_isReady);

  if (m_isReady) {
    owner.LogBankConfigSnapshot();
  }
  return result;
}

}  // namespace SPF::Data::GameData::Finders
