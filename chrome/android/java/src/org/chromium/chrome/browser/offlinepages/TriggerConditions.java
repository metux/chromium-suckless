// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages;

/** Set of system conditions to trigger background processing. */
public class TriggerConditions {
    private final boolean mRequirePowerConnected;
    private final boolean mRequireUnmeteredNetwork;

    /**
     * Creates set of device, network, and power conditions for triggering processing.
     * @param requirePowerConnected whether to require that device is connected to power
     * @param requireUnmeteredNetwork whether to require connection to unmetered network
     */
    public TriggerConditions(boolean requirePowerConnected, int foo,
            boolean requireUnmeteredNetwork) {
        mRequirePowerConnected = requirePowerConnected;
        mRequireUnmeteredNetwork = requireUnmeteredNetwork;
    }

    /** Returns whether connection to power is required. */
    public boolean requirePowerConnected() {
        return mRequirePowerConnected;
    }

    /** Returns whether connection to an unmetered network is required. */
    public boolean requireUnmeteredNetwork() {
        return mRequireUnmeteredNetwork;
    }

    @Override
    public int hashCode() {
        int hash = 13;
        hash = hash * 31 + (mRequirePowerConnected ? 1 : 0);
        hash = hash * 31 + (mRequireUnmeteredNetwork ? 1 : 0);
        return hash;
    }

    @Override
    public boolean equals(Object other) {
        if (!(other instanceof TriggerConditions)) return false;
        TriggerConditions otherTriggerConditions = (TriggerConditions) other;
        return mRequirePowerConnected == otherTriggerConditions.mRequirePowerConnected
                && mRequireUnmeteredNetwork == otherTriggerConditions.mRequireUnmeteredNetwork;
    }
}
