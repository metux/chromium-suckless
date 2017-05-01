// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_RENDERER_AW_CONTENT_RENDERER_CLIENT_H_
#define ANDROID_WEBVIEW_RENDERER_AW_CONTENT_RENDERER_CLIENT_H_

#include <memory>
#include <string>

#include "android_webview/renderer/aw_render_thread_observer.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "components/web_restrictions/interfaces/web_restrictions.mojom.h"
#include "content/public/renderer/content_renderer_client.h"

#if defined(ENABLE_SPELLCHECK)
class SpellCheck;
class SpellCheckProvider;
#endif

namespace visitedlink {
class VisitedLinkSlave;
}

namespace android_webview {

class AwContentRendererClient : public content::ContentRendererClient {
 public:
  AwContentRendererClient();
  ~AwContentRendererClient() override;

  // ContentRendererClient implementation.
  void RenderThreadStarted() override;
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void RenderViewCreated(content::RenderView* render_view) override;
  bool HasErrorPage(int http_status_code, std::string* error_domain) override;
  void GetNavigationErrorStrings(content::RenderFrame* render_frame,
                                 const blink::WebURLRequest& failed_request,
                                 const blink::WebURLError& error,
                                 std::string* error_html,
                                 base::string16* error_description) override;
  unsigned long long VisitedLinkHash(const char* canonical_url,
                                     size_t length) override;
  bool IsLinkVisited(unsigned long long link_hash) override;
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems)
      override;

  bool HandleNavigation(content::RenderFrame* render_frame,
                        bool is_content_initiated,
                        int opener_id,
                        blink::WebFrame* frame,
                        const blink::WebURLRequest& request,
                        blink::WebNavigationType type,
                        blink::WebNavigationPolicy default_policy,
                        bool is_redirect) override;
  bool ShouldUseMediaPlayerForURL(const GURL& url) override;

 private:
  std::unique_ptr<AwRenderThreadObserver> aw_render_thread_observer_;
  std::unique_ptr<visitedlink::VisitedLinkSlave> visited_link_slave_;
  web_restrictions::mojom::WebRestrictionsPtr web_restrictions_service_;

#if defined(ENABLE_SPELLCHECK)
  std::unique_ptr<SpellCheck> spellcheck_;
#endif

  DISALLOW_COPY_AND_ASSIGN(AwContentRendererClient);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_RENDERER_AW_CONTENT_RENDERER_CLIENT_H_