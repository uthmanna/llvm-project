//===--- IRPrintingPasses.cpp - Module and Function printing passes -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// PrintModulePass and PrintFunctionPass implementations.
//
//===----------------------------------------------------------------------===//

#include "llvm/IRPrinter/IRPrintingPasses.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/ModuleSummaryAnalysis.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PrintPasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

PrintModulePass::PrintModulePass() : OS(dbgs()) {}
PrintModulePass::PrintModulePass(raw_ostream &OS, const std::string &Banner,
                                 bool ShouldPreserveUseListOrder,
                                 bool EmitSummaryIndex)
    : OS(OS), Banner(Banner),
      ShouldPreserveUseListOrder(ShouldPreserveUseListOrder),
      EmitSummaryIndex(EmitSummaryIndex) {}

PreservedAnalyses PrintModulePass::run(Module &M, ModuleAnalysisManager &AM) {
  // Remove intrinsic declarations when printing in the new format.
  // TODO: consider removing this now that debug intrinsics are gone.
  M.removeDebugIntrinsicDeclarations();

  if (llvm::isFunctionInPrintList("*")) {
    if (!Banner.empty())
      OS << Banner << "\n";
    M.print(OS, nullptr, ShouldPreserveUseListOrder);
  } else {
    bool BannerPrinted = false;
    for (const auto &F : M.functions()) {
      if (llvm::isFunctionInPrintList(F.getName())) {
        if (!BannerPrinted && !Banner.empty()) {
          OS << Banner << "\n";
          BannerPrinted = true;
        }
        F.print(OS);
      }
    }
  }

  ModuleSummaryIndex *Index =
      EmitSummaryIndex ? &(AM.getResult<ModuleSummaryIndexAnalysis>(M))
                       : nullptr;
  if (Index) {
    if (Index->modulePaths().empty())
      Index->addModule("");
    Index->print(OS);
  }

  return PreservedAnalyses::all();
}

PrintFunctionPass::PrintFunctionPass() : OS(dbgs()) {}
PrintFunctionPass::PrintFunctionPass(raw_ostream &OS, const std::string &Banner)
    : OS(OS), Banner(Banner) {}

PreservedAnalyses PrintFunctionPass::run(Function &F,
                                         FunctionAnalysisManager &) {
  if (isFunctionInPrintList(F.getName())) {
    if (forcePrintModuleIR())
      OS << Banner << " (function: " << F.getName() << ")\n" << *F.getParent();
    else
      OS << Banner << '\n' << static_cast<Value &>(F);
  }

  return PreservedAnalyses::all();
}
