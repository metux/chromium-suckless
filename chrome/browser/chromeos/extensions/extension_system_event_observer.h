// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_EXTENSION_SYSTEM_EVENT_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_EXTENSION_SYSTEM_EVENT_OBSERVER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chromeos/dbus/session_manager_client.h"

namespace chromeos {

// Dispatches extension events in response to system events.
class ExtensionSystemEventObserver : 
                                     public SessionManagerClient::Observer {
 public:
  // This class registers/unregisters itself as an observer in ctor/dtor.
  ExtensionSystemEventObserver();
  ~ExtensionSystemEventObserver() override;

  // SessionManagerClient::Observer override.
  void ScreenIsUnlocked() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtensionSystemEventObserver);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_EXTENSION_SYSTEM_EVENT_OBSERVER_H_
