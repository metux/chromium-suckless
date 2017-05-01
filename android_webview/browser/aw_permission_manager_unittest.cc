// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_permission_manager.h"

#include <list>
#include <memory>

#include "android_webview/browser/aw_browser_permission_request_delegate.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/permission_manager.h"
#include "content/public/browser/permission_type.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::MakeUnique;
using blink::mojom::PermissionStatus;
using content::PermissionType;

namespace android_webview {

namespace {

int kNoPendingOperation = -1;

int kRenderProcessIDForTesting = 8;
int kRenderFrameIDForTesting = 19;
const char kEmbeddingOrigin[] = "https://www.google.com";
const char kRequestingOrigin1[] = "https://www.google.com";
const char kRequestingOrigin2[] = "https://www.chromium.org";

class AwBrowserPermissionRequestDelegateForTesting
    : public AwBrowserPermissionRequestDelegate {
 public:
  void EnqueueResponse(const std::string& origin,
                       PermissionType type,
                       bool grant) {
    for (auto it = request_.begin(); it != request_.end(); ++it) {
      if ((*it)->type != type || (*it)->origin != GURL(origin))
        continue;
      const base::Callback<void(bool)> callback = (*it)->callback;
      request_.erase(it);
      callback.Run(grant);
      return;
    }
    response_.push_back(MakeUnique<Response>(origin, type, grant));
  }

  // AwBrowserPermissionRequestDelegate:
  void RequestProtectedMediaIdentifierPermission(
      const GURL& origin,
      const base::Callback<void(bool)>& callback) override {}

  void CancelProtectedMediaIdentifierPermissionRequests(
      const GURL& origin) override {}

  void RequestGeolocationPermission(
      const GURL& origin,
      const base::Callback<void(bool)>& callback) override {
    RequestPermission(origin, PermissionType::GEOLOCATION, callback);
  }

  void CancelGeolocationPermissionRequests(const GURL& origin) override {
    CancelPermission(origin, PermissionType::GEOLOCATION);
  }

 private:
  void RequestPermission(const GURL& origin,
                         PermissionType type,
                         const base::Callback<void(bool)>& callback) {
    for (auto it = response_.begin(); it != response_.end(); ++it) {
      if ((*it)->type != type || (*it)->origin != origin)
        continue;
      bool grant = (*it)->grant;
      response_.erase(it);
      callback.Run(grant);
      return;
    }
    request_.push_back(MakeUnique<Request>(origin, type, callback));
  }

  void CancelPermission(const GURL& origin, PermissionType type) {
    for (auto it = request_.begin(); it != request_.end(); ++it) {
      if ((*it)->type != type || (*it)->origin != origin)
        continue;
      request_.erase(it);
      return;
    }
    NOTREACHED();
  }

 private:
  struct Request {
    GURL origin;
    PermissionType type;
    base::Callback<void(bool)> callback;

    Request(const GURL& origin,
            PermissionType type,
            const base::Callback<void(bool)>& callback)
        : origin(origin), type(type), callback(callback) {}
  };

  struct Response {
    GURL origin;
    PermissionType type;
    bool grant;

    Response(const std::string& origin, PermissionType type, bool grant)
        : origin(GURL(origin)), type(type), grant(grant) {}

  };

  std::list<std::unique_ptr<Response>> response_;
  std::list<std::unique_ptr<Request>> request_;
};

class AwPermissionManagerForTesting : public AwPermissionManager {
 public:
  ~AwPermissionManagerForTesting() override {
    // Call CancelPermissionRequests() from here so that it calls virtual
    // methods correctly.
    CancelPermissionRequests();
  }

  void EnqueuePermissionResponse(const std::string& origin,
                                 PermissionType type,
                                 bool grant) {
    delegate()->EnqueueResponse(origin, type, grant);
  }

 private:
  AwBrowserPermissionRequestDelegateForTesting* delegate() {
    if (!delegate_)
      delegate_.reset(new AwBrowserPermissionRequestDelegateForTesting);
    return delegate_.get();
  }

  // AwPermissionManager:
  int GetRenderProcessID(content::RenderFrameHost* render_frame_host) override {
    return kRenderProcessIDForTesting;
  }

  int GetRenderFrameID(content::RenderFrameHost* render_frame_host) override {
    return kRenderFrameIDForTesting;
  }

  GURL LastCommittedOrigin(
      content::RenderFrameHost* render_frame_host) override {
    return GURL(kEmbeddingOrigin);
  }

  AwBrowserPermissionRequestDelegate* GetDelegate(
      int render_process_id,
      int render_frame_id) override {
    CHECK_EQ(kRenderProcessIDForTesting, render_process_id);
    CHECK_EQ(kRenderFrameIDForTesting, render_frame_id);
    return delegate();
  }

  std::unique_ptr<AwBrowserPermissionRequestDelegateForTesting> delegate_;
};

class AwPermissionManagerTest : public testing::Test {
 public:
  AwPermissionManagerTest()
      : render_frame_host(nullptr) {}

  void PermissionRequestResponse(int id, const PermissionStatus status) {
    resolved_permission_status.push_back(status);
    resolved_permission_request_id.push_back(id);
  }

  void PermissionsRequestResponse(int id,
                                  const std::vector<PermissionStatus>& status) {
    resolved_permission_status.insert(resolved_permission_status.end(),
                                      status.begin(), status.end());
    for (size_t i = 0; i < status.size(); ++i)
      resolved_permission_request_id.push_back(id);
  }

 protected:
  void SetUp() override { manager.reset(new AwPermissionManagerForTesting); }
  void TearDown() override { manager.reset(); }

  void EnqueuePermissionResponse(const std::string& origin,
                                 PermissionType type,
                                 bool grant) {
    CHECK(manager);
    manager->EnqueuePermissionResponse(origin, type, grant);
  }

  std::unique_ptr<AwPermissionManagerForTesting> manager;

  // Use nullptr for testing. AwPermissionManagerForTesting override all methods
  // that touch RenderFrameHost to work with nullptr.
  content::RenderFrameHost* render_frame_host;

  std::vector<PermissionStatus> resolved_permission_status;
  std::vector<int> resolved_permission_request_id;
};

// Test the case a delegate is called, and it resolves the permission
// synchronously.
TEST_F(AwPermissionManagerTest, SinglePermissionRequestIsGrantedSynchronously) {
  // Permission should be granted in this scenario.
  manager->EnqueuePermissionResponse(kRequestingOrigin1,
                                     PermissionType::GEOLOCATION, true);
  int request_id = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 0));
  EXPECT_EQ(kNoPendingOperation, request_id);
  ASSERT_EQ(1u, resolved_permission_status.size());
  EXPECT_EQ(PermissionStatus::GRANTED, resolved_permission_status[0]);

  // Permission should not be granted in this scenario.
  manager->EnqueuePermissionResponse(kRequestingOrigin1,
                                     PermissionType::GEOLOCATION, false);
  request_id = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 0));
  EXPECT_EQ(kNoPendingOperation, request_id);
  ASSERT_EQ(2u, resolved_permission_status.size());
  EXPECT_EQ(PermissionStatus::DENIED, resolved_permission_status[1]);
}

// Test the case a delegate is called, and it resolves the permission
// asynchronously.
TEST_F(AwPermissionManagerTest,
       SinglePermissionRequestIsGrantedAsynchronously) {
  int request_id = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 0));
  EXPECT_NE(kNoPendingOperation, request_id);
  EXPECT_EQ(0u, resolved_permission_status.size());

  // This will resolve the permission.
  manager->EnqueuePermissionResponse(kRequestingOrigin1,
                                     PermissionType::GEOLOCATION, true);

  ASSERT_EQ(1u, resolved_permission_status.size());
  EXPECT_EQ(PermissionStatus::GRANTED, resolved_permission_status[0]);
}

// Test the case a delegate is called, and the manager is deleted before the
// delegate callback is invoked.
TEST_F(AwPermissionManagerTest, ManagerIsDeletedWhileDelegateProcesses) {
  int request_id = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 0));

  EXPECT_NE(kNoPendingOperation, request_id);
  EXPECT_EQ(0u, resolved_permission_status.size());

  // Delete the manager.
  manager.reset();

  // All requests are cancelled internally.
  EXPECT_EQ(0u, resolved_permission_status.size());
}

// Test the case multiple permissions are requested for the same origin, and the
// second permission is also resolved when the first permission is resolved.
TEST_F(AwPermissionManagerTest,
       MultiplePermissionRequestsAreGrantedTogether) {
  int request_1 = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 1));
  EXPECT_NE(kNoPendingOperation, request_1);

  int request_2 = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 2));
  EXPECT_NE(kNoPendingOperation, request_2);

  EXPECT_NE(request_1, request_2);
  EXPECT_EQ(0u, resolved_permission_status.size());

  // This will resolve the permission.
  manager->EnqueuePermissionResponse(kRequestingOrigin1,
                                     PermissionType::GEOLOCATION, true);

  ASSERT_EQ(2u, resolved_permission_status.size());
  EXPECT_EQ(PermissionStatus::GRANTED, resolved_permission_status[0]);
  EXPECT_EQ(PermissionStatus::GRANTED, resolved_permission_status[1]);
}

// Test the case multiple permissions are requested for different origins, and
// each permission is resolved respectively in the requested order.
TEST_F(AwPermissionManagerTest,
       MultiplePermissionRequestsAreGrantedRespectively) {
  int request_1 = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 1));
  EXPECT_NE(kNoPendingOperation, request_1);

  int request_2 = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin2),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 2));
  EXPECT_NE(kNoPendingOperation, request_2);

  EXPECT_NE(request_1, request_2);
  EXPECT_EQ(0u, resolved_permission_status.size());

  // This will resolve the first request.
  manager->EnqueuePermissionResponse(kRequestingOrigin1,
                                     PermissionType::GEOLOCATION, true);

  ASSERT_EQ(1u, resolved_permission_status.size());
  EXPECT_EQ(PermissionStatus::GRANTED, resolved_permission_status[0]);
  EXPECT_EQ(1, resolved_permission_request_id[0]);

  // This will resolve the second request.
  manager->EnqueuePermissionResponse(kRequestingOrigin2,
                                     PermissionType::GEOLOCATION, false);
  EXPECT_EQ(PermissionStatus::DENIED, resolved_permission_status[1]);
  EXPECT_EQ(2, resolved_permission_request_id[1]);
}

// Test the case CancelPermissionRequest is called for an invalid request.
TEST_F(AwPermissionManagerTest, InvalidRequestIsCancelled) {
  manager->CancelPermissionRequest(0);
}

// Test the case a delegate is called, and cancelled.
TEST_F(AwPermissionManagerTest, SinglePermissionRequestIsCancelled) {
  int request_id = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 0));
  EXPECT_NE(kNoPendingOperation, request_id);
  EXPECT_EQ(0u, resolved_permission_status.size());

  manager->CancelPermissionRequest(request_id);
  EXPECT_EQ(0u, resolved_permission_status.size());

  // This should not resolve the permission.
  manager->EnqueuePermissionResponse(kRequestingOrigin1,
                                     PermissionType::GEOLOCATION, true);
  EXPECT_EQ(0u, resolved_permission_status.size());
}

// Test the case multiple permissions are requested, and cancelled as follow.
//  1. Permission A is requested.
//  2. Permission A is requested for the same origin again.
//  3. The first request is cancelled.
TEST_F(AwPermissionManagerTest, ComplicatedCancelScenario1) {
  int request_1 = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 1));
  EXPECT_NE(kNoPendingOperation, request_1);
  EXPECT_EQ(0u, resolved_permission_status.size());

  int request_2 = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 2));
  EXPECT_NE(kNoPendingOperation, request_2);
  EXPECT_EQ(0u, resolved_permission_status.size());

  EXPECT_NE(request_1, request_2);

  manager->CancelPermissionRequest(request_1);
  EXPECT_EQ(0u, resolved_permission_status.size());

  // This should resolve the second request.
  manager->EnqueuePermissionResponse(kRequestingOrigin1,
                                     PermissionType::GEOLOCATION, true);
  EXPECT_EQ(1u, resolved_permission_status.size());
  EXPECT_EQ(PermissionStatus::GRANTED, resolved_permission_status[0]);
  EXPECT_EQ(2, resolved_permission_request_id[0]);
}

// Test the case multiple permissions are requested, and cancelled as follow.
//  1. Permission A is requested.
//  2. Permission A is requested for a different origin.
//  3. The first request is cancelled.
TEST_F(AwPermissionManagerTest, ComplicatedCancelScenario2) {
  int request_1 = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin1),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 1));
  EXPECT_NE(kNoPendingOperation, request_1);
  EXPECT_EQ(0u, resolved_permission_status.size());

  int request_2 = manager->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host, GURL(kRequestingOrigin2),
      true, base::Bind(&AwPermissionManagerTest::PermissionRequestResponse,
                       base::Unretained(this), 2));
  EXPECT_NE(kNoPendingOperation, request_2);
  EXPECT_EQ(0u, resolved_permission_status.size());

  EXPECT_NE(request_1, request_2);

  manager->CancelPermissionRequest(request_1);
  EXPECT_EQ(0u, resolved_permission_status.size());

  // This should not resolve the first request.
  manager->EnqueuePermissionResponse(kRequestingOrigin1,
                                     PermissionType::GEOLOCATION, true);
  EXPECT_EQ(0u, resolved_permission_status.size());

  // This should resolve the second request.
  manager->EnqueuePermissionResponse(kRequestingOrigin2,
                                     PermissionType::GEOLOCATION, true);
  EXPECT_EQ(1u, resolved_permission_status.size());
  EXPECT_EQ(PermissionStatus::GRANTED, resolved_permission_status[0]);
  EXPECT_EQ(2, resolved_permission_request_id[0]);
}

}  // namespace

}  // namespace android_webview
