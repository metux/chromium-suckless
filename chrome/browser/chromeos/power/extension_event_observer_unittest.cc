// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/extension_event_observer.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chromeos/login/users/scoped_user_manager_enabler.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "chrome/common/extensions/api/gcm.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_renderer_host.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_host_observer.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/test/test_screen.h"
#include "ui/display/screen.h"

namespace chromeos {

class ExtensionEventObserverTest : public ::testing::Test {
 public:
  ExtensionEventObserverTest()
      : test_screen_(aura::TestScreen::Create(gfx::Size())),
        fake_user_manager_(new FakeChromeUserManager()),
        scoped_user_manager_enabler_(fake_user_manager_) {

    profile_manager_.reset(
        new TestingProfileManager(TestingBrowserProcess::GetGlobal()));

    extension_event_observer_.reset(new ExtensionEventObserver());
    test_api_ = extension_event_observer_->CreateTestApi();
  }

  ~ExtensionEventObserverTest() override {
    extension_event_observer_.reset();
    profile_manager_.reset();
    DBusThreadManager::Shutdown();
  }

  // ::testing::Test overrides.
  void SetUp() override {
    ::testing::Test::SetUp();

    display::Screen::SetScreenInstance(test_screen_.get());

    // Must be called from ::testing::Test::SetUp.
    ASSERT_TRUE(profile_manager_->SetUp());

    const char kUserProfile[] = "profile1@example.com";
    const AccountId account_id(AccountId::FromUserEmail(kUserProfile));
    fake_user_manager_->AddUser(account_id);
    fake_user_manager_->LoginUser(account_id);
    profile_ =
        profile_manager_->CreateTestingProfile(account_id.GetUserEmail());

    profile_manager_->SetLoggedIn(true);
  }
  void TearDown() override {
    profile_ = NULL;
    profile_manager_->DeleteAllTestingProfiles();
    display::Screen::SetScreenInstance(nullptr);
    ::testing::Test::TearDown();
  }

 protected:
  scoped_refptr<extensions::Extension> CreateApp(const std::string& name,
                                                 bool uses_gcm) {
    scoped_refptr<extensions::Extension> app =
        extensions::ExtensionBuilder()
            .SetManifest(
                extensions::DictionaryBuilder()
                    .Set("name", name)
                    .Set("version", "1.0.0")
                    .Set("manifest_version", 2)
                    .Set("app", extensions::DictionaryBuilder()
                                    .Set("background",
                                         extensions::DictionaryBuilder()
                                             .Set("scripts",
                                                  extensions::ListBuilder()
                                                      .Append("background.js")
                                                      .Build())
                                             .Build())
                                    .Build())
                    .Set("permissions", extensions::ListBuilder()
                                            .Append(uses_gcm ? "gcm" : "")
                                            .Build())
                    .Build())
            .Build();

    created_apps_.push_back(app);

    return app;
  }

  extensions::ExtensionHost* CreateHostForApp(Profile* profile,
                                              extensions::Extension* app) {
    extensions::ProcessManager::Get(profile)->CreateBackgroundHost(
        app, extensions::BackgroundInfo::GetBackgroundURL(app));
    base::RunLoop().RunUntilIdle();

    return extensions::ProcessManager::Get(profile)
        ->GetBackgroundHostForExtension(app->id());
  }

  std::unique_ptr<ExtensionEventObserver> extension_event_observer_;
  std::unique_ptr<ExtensionEventObserver::TestApi> test_api_;

  // Owned by |profile_manager_|.
  TestingProfile* profile_;
  std::unique_ptr<TestingProfileManager> profile_manager_;

 private:
  std::unique_ptr<aura::TestScreen> test_screen_;
  content::TestBrowserThreadBundle browser_thread_bundle_;

  // Needed to ensure we don't end up creating actual RenderViewHosts
  // and RenderProcessHosts.
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;

  // Chrome OS needs extra services to run in the following order.
  ScopedTestDeviceSettingsService test_device_settings_service_;
  ScopedTestCrosSettings test_cros_settings_;

  // Owned by |scoped_user_manager_enabler_|.
  FakeChromeUserManager* fake_user_manager_;
  ScopedUserManagerEnabler scoped_user_manager_enabler_;

  std::vector<scoped_refptr<extensions::Extension>> created_apps_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionEventObserverTest);
};

}  // namespace chromeos
