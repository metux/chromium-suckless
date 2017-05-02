// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_desktop_controller_aura.h"

#include <memory>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "ui/aura/test/aura_test_base.h"

#if defined(OS_CHROMEOS)
#include "chromeos/dbus/dbus_thread_manager.h"
#endif

namespace extensions {

class ShellDesktopControllerAuraTest : public aura::test::AuraTestBase {
 public:
  ShellDesktopControllerAuraTest()
      {
  }
  ~ShellDesktopControllerAuraTest() override {}

  void SetUp() override {
    aura::test::AuraTestBase::SetUp();
    controller_.reset(new ShellDesktopControllerAura());
  }

  void TearDown() override {
    controller_.reset();
    aura::test::AuraTestBase::TearDown();
#if defined(OS_CHROMEOS)
    chromeos::DBusThreadManager::Shutdown();
#endif
  }

 protected:
  std::unique_ptr<ShellDesktopControllerAura> controller_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ShellDesktopControllerAuraTest);
};

}  // namespace extensions
