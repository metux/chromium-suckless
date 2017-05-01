// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/browser_lifetime_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chrome/browser/lifetime/application_lifetime.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"
#endif  // defined(OS_CHROMEOS)

namespace settings {

BrowserLifetimeHandler::BrowserLifetimeHandler() {}

BrowserLifetimeHandler::~BrowserLifetimeHandler() {}

void BrowserLifetimeHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("restart",
      base::Bind(&BrowserLifetimeHandler::HandleRestart,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("relaunch",
      base::Bind(&BrowserLifetimeHandler::HandleRelaunch,
                 base::Unretained(this)));
#if defined(OS_CHROMEOS)
  web_ui()->RegisterMessageCallback("signOutAndRestart",
      base::Bind(&BrowserLifetimeHandler::HandleSignOutAndRestart,
                 base::Unretained(this)));
#endif  // defined(OS_CHROMEOS)
}

void BrowserLifetimeHandler::HandleRestart(
    const base::ListValue* args) {
  chrome::AttemptRestart();
}

void BrowserLifetimeHandler::HandleRelaunch(
    const base::ListValue* args) {
  chrome::AttemptRelaunch();
}

#if defined(OS_CHROMEOS)
void BrowserLifetimeHandler::HandleSignOutAndRestart(
    const base::ListValue* args) {
  chrome::AttemptUserExit();
}

#endif  // defined(OS_CHROMEOS)

}  // namespace settings
