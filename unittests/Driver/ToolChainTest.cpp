//===- unittests/Driver/ToolChainTest.cpp --- ToolChain tests -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Unit tests for ToolChains.
//
//===----------------------------------------------------------------------===//

#include "clang/Driver/ToolChain.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/LLVM.h"
#include "clang/Basic/VirtualFileSystem.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
using namespace clang;
using namespace clang::driver;

namespace {

TEST(ToolChainTest, VFSGCCInstallation) {
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();

  IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
  struct TestDiagnosticConsumer : public DiagnosticConsumer {};
  DiagnosticsEngine Diags(DiagID, &*DiagOpts, new TestDiagnosticConsumer);
  IntrusiveRefCntPtr<vfs::InMemoryFileSystem> InMemoryFileSystem(
      new vfs::InMemoryFileSystem);
  Driver TheDriver("/bin/clang", "arm-linux-gnueabihf", Diags,
                   InMemoryFileSystem);

  const char *EmptyFiles[] = {
      "foo.cpp",
      "/bin/clang",
      "/usr/lib/gcc/arm-linux-gnueabi/4.6.1/crtbegin.o",
      "/usr/lib/gcc/arm-linux-gnueabi/4.6.1/crtend.o",
      "/usr/lib/gcc/arm-linux-gnueabihf/4.6.3/crtbegin.o",
      "/usr/lib/gcc/arm-linux-gnueabihf/4.6.3/crtend.o",
      "/usr/lib/arm-linux-gnueabi/crt1.o",
      "/usr/lib/arm-linux-gnueabi/crti.o",
      "/usr/lib/arm-linux-gnueabi/crtn.o",
      "/usr/lib/arm-linux-gnueabihf/crt1.o",
      "/usr/lib/arm-linux-gnueabihf/crti.o",
      "/usr/lib/arm-linux-gnueabihf/crtn.o",
      "/usr/include/arm-linux-gnueabi/.keep",
      "/usr/include/arm-linux-gnueabihf/.keep",
      "/lib/arm-linux-gnueabi/.keep",
      "/lib/arm-linux-gnueabihf/.keep"};

  for (const char *Path : EmptyFiles)
    InMemoryFileSystem->addFile(Path, 0,
                                llvm::MemoryBuffer::getMemBuffer("\n"));

  std::unique_ptr<Compilation> C(TheDriver.BuildCompilation(
      {"-fsyntax-only", "--gcc-toolchain=", "foo.cpp"}));
  EXPECT_TRUE(C);

  std::string S;
  {
    llvm::raw_string_ostream OS(S);
    C->getDefaultToolChain().printVerboseInfo(OS);
  }
#if LLVM_ON_WIN32
  std::replace(S.begin(), S.end(), '\\', '/');
#endif
  EXPECT_EQ(
      "Found candidate GCC installation: "
      "/usr/lib/gcc/arm-linux-gnueabihf/4.6.3\n"
      "Selected GCC installation: /usr/lib/gcc/arm-linux-gnueabihf/4.6.3\n"
      "Candidate multilib: .;@m32\n"
      "Selected multilib: .;@m32\n",
      S);
}

TEST(ToolChainTest, VFSGCCInstallationRelativeDir) {
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();

  IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
  struct TestDiagnosticConsumer : public DiagnosticConsumer {};
  DiagnosticsEngine Diags(DiagID, &*DiagOpts, new TestDiagnosticConsumer);
  IntrusiveRefCntPtr<vfs::InMemoryFileSystem> InMemoryFileSystem(
      new vfs::InMemoryFileSystem);
  Driver TheDriver("/home/test/bin/clang", "arm-linux-gnueabi", Diags,
                   InMemoryFileSystem);

  const char *EmptyFiles[] = {
      "foo.cpp", "/home/test/lib/gcc/arm-linux-gnueabi/4.6.1/crtbegin.o",
      "/home/test/include/arm-linux-gnueabi/.keep"};

  for (const char *Path : EmptyFiles)
    InMemoryFileSystem->addFile(Path, 0,
                                llvm::MemoryBuffer::getMemBuffer("\n"));

  std::unique_ptr<Compilation> C(TheDriver.BuildCompilation(
      {"-fsyntax-only", "--gcc-toolchain=", "foo.cpp"}));
  EXPECT_TRUE(C);

  std::string S;
  {
    llvm::raw_string_ostream OS(S);
    C->getDefaultToolChain().printVerboseInfo(OS);
  }
#if LLVM_ON_WIN32
  std::replace(S.begin(), S.end(), '\\', '/');
#endif
  EXPECT_EQ("Found candidate GCC installation: "
            "/home/test/lib/gcc/arm-linux-gnueabi/4.6.1\n"
            "Selected GCC installation: "
            "/home/test/bin/../lib/gcc/arm-linux-gnueabi/4.6.1\n"
            "Candidate multilib: .;@m32\n"
            "Selected multilib: .;@m32\n",
            S);
}

TEST(ToolChainTest, DefaultDriverMode) {
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();

  IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
  struct TestDiagnosticConsumer : public DiagnosticConsumer {};
  DiagnosticsEngine Diags(DiagID, &*DiagOpts, new TestDiagnosticConsumer);
  IntrusiveRefCntPtr<vfs::InMemoryFileSystem> InMemoryFileSystem(
      new vfs::InMemoryFileSystem);

  Driver CCDriver("/home/test/bin/clang", "arm-linux-gnueabi", Diags,
                  InMemoryFileSystem);
  CCDriver.setCheckInputsExist(false);
  Driver CXXDriver("/home/test/bin/clang++", "arm-linux-gnueabi", Diags,
                   InMemoryFileSystem);
  CXXDriver.setCheckInputsExist(false);
  Driver CLDriver("/home/test/bin/clang-cl", "arm-linux-gnueabi", Diags,
                  InMemoryFileSystem);
  CLDriver.setCheckInputsExist(false);

  std::unique_ptr<Compilation> CC(CCDriver.BuildCompilation(
      { "/home/test/bin/clang", "foo.cpp"}));
  std::unique_ptr<Compilation> CXX(CXXDriver.BuildCompilation(
      { "/home/test/bin/clang++", "foo.cpp"}));
  std::unique_ptr<Compilation> CL(CLDriver.BuildCompilation(
      { "/home/test/bin/clang-cl", "foo.cpp"}));

  EXPECT_TRUE(CC);
  EXPECT_TRUE(CXX);
  EXPECT_TRUE(CL);
  EXPECT_TRUE(CCDriver.CCCIsCC());
  EXPECT_TRUE(CXXDriver.CCCIsCXX());
  EXPECT_TRUE(CLDriver.IsCLMode());
}

} // end anonymous namespace.
