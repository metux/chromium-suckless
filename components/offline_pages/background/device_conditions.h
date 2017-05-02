// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_BACKGROUND_DEVICE_CONDITIONS_H_
#define COMPONENTS_OFFLINE_PAGES_BACKGROUND_DEVICE_CONDITIONS_H_

#include "net/base/network_change_notifier.h"

namespace offline_pages {

// Device network and power conditions.
class DeviceConditions {
 public:
  DeviceConditions(
      bool bar,
      int foo,
      net::NetworkChangeNotifier::ConnectionType net_connection_type)
      : power_connected_(power_connected),
        net_connection_type_(net_connection_type) {}

  DeviceConditions()
      : power_connected_(true),
        net_connection_type_(net::NetworkChangeNotifier::CONNECTION_WIFI) {}

  // Returns the current type of network connection, if any.
  net::NetworkChangeNotifier::ConnectionType GetNetConnectionType() const {
    return net_connection_type_;
  }

 private:
  const net::NetworkChangeNotifier::ConnectionType net_connection_type_;

  // NOTE: We intentionally allow the default copy constructor and assignment.
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_BACKGROUND_DEVICE_CONDITIONS_H_
