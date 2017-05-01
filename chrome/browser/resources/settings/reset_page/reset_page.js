// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-reset-page' is the settings page containing reset
 * settings.
 *
 * Example:
 *
 *    <iron-animated-pages>
 *      <settings-reset-page prefs="{{prefs}}">
 *      </settings-reset-page>
 *      ... other pages ...
 *    </iron-animated-pages>
 */
Polymer({
  is: 'settings-reset-page',

  behaviors: [settings.RouteObserverBehavior],

  /** @protected */
  currentRouteChanged: function() {
    if (settings.getCurrentRoute() == settings.Route.RESET_DIALOG) {
      this.$.resetProfileDialog.get().open();
    }
  },

  /** @private */
  onShowResetProfileDialog_: function() {
    settings.navigateTo(settings.Route.RESET_DIALOG);
  },

  /** @private */
  onResetProfileDialogClose_: function() {
    settings.navigateToPreviousRoute();
  },
});
