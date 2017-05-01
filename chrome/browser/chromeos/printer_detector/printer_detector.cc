// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printer_detector/printer_detector.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/notification_delegate.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "chrome/common/extensions/api/webstore_widget_private.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "device/core/device_client.h"
#include "device/usb/usb_device.h"
#include "device/usb/usb_device_filter.h"
#include "device/usb/usb_ids.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/api/printer_provider/usb_printer_manifest_data.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/one_shot_event.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/permissions/usb_device_permission.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace chromeos {

PrinterDetector::PrinterDetector(Profile* profile)
    : profile_(profile),
      notification_ui_manager_(nullptr),
      observer_(this),
      weak_ptr_factory_(this) {
  extensions::ExtensionSystem::Get(profile)->ready().Post(
      FROM_HERE,
      base::Bind(&PrinterDetector::Initialize, weak_ptr_factory_.GetWeakPtr()));
}

PrinterDetector::~PrinterDetector() {
}

void PrinterDetector::Shutdown() {
}

void PrinterDetector::Initialize() {
}

void PrinterDetector::OnDeviceAdded(scoped_refptr<device::UsbDevice> device) {
}

void PrinterDetector::SetNotificationUIManagerForTesting(
    NotificationUIManager* manager) {
  notification_ui_manager_ = manager;
}

}  // namespace chromeos
