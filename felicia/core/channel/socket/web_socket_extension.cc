// Copyright (c) 2019 The Felicia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "felicia/core/channel/socket/web_socket_extension.h"

#include "third_party/chromium/base/strings/string_util.h"

#include "felicia/core/channel/socket/permessage_deflate.h"

namespace felicia {

WebSocketExtensionInterface::WebSocketExtensionInterface() = default;
WebSocketExtensionInterface::~WebSocketExtensionInterface() = default;

WebSocketExtension::WebSocketExtension() {
  extensions_[PermessageDeflate::kKey] = std::make_unique<PermessageDeflate>();
}

bool WebSocketExtension::Negotiate(
    const std::string& extensions, const channel::WSSettings& settings,
    std::string* response,
    std::vector<WebSocketExtensionInterface*>* accepted_extensions) {
  base::StringTokenizer extension(extensions.cbegin(), extensions.cend(), ",");
  while (extension.GetNext()) {
    base::StringTokenizer params(extension.token_begin(), extension.token_end(),
                                 ";");

    if (!params.GetNext()) return false;

    base::StringPiece key = base::TrimWhitespaceASCII(
        params.token_piece(), base::TrimPositions::TRIM_ALL);

    auto it = extensions_.find(key);
    if (it == extensions_.end()) {
      // TODO(chokobole): should we continue? or return false?
      DLOG(ERROR) << "No extension for " << key;
      return false;
    }

    // TODO(chokobole): If extension is added more, then maybe negotiate
    // followings. At this moment, not to negotiate fallback one, return
    // early here.
    if (it->second->Negotiate(params, settings, response)) {
      accepted_extensions->push_back(it->second.get());
      return true;
    }
  }

  return false;
}

}  // namespace felicia