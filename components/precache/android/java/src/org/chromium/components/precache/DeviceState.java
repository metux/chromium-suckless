// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.precache;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import org.chromium.base.VisibleForTesting;

/**
 * Utility class that provides information about the current state of the device.
 */
public class DeviceState {
    private static DeviceState sDeviceState = null;

    /** Disallow Construction of DeviceState objects. Use {@link #getInstance()} instead to create
     * a singleton instance.
     */
    protected DeviceState() {}

    public static DeviceState getInstance() {
        if (sDeviceState == null) sDeviceState = new DeviceState();
        return sDeviceState;
    }

    protected NetworkInfoDelegateFactory mNetworkInfoDelegateFactory =
            new NetworkInfoDelegateFactory();

    @VisibleForTesting
    void setNetworkInfoDelegateFactory(NetworkInfoDelegateFactory factory) {
        mNetworkInfoDelegateFactory = factory;
    }

    /** @return whether the currently active network is unmetered. */
    public boolean isUnmeteredNetworkAvailable(Context context) {
        NetworkInfoDelegate networkInfo =
                mNetworkInfoDelegateFactory.getNetworkInfoDelegate(context);
        return (networkInfo.isValid()
                && networkInfo.isAvailable()
                && networkInfo.isConnected()
                && !networkInfo.isRoaming()
                && !networkInfo.isActiveNetworkMetered());
    }
}

