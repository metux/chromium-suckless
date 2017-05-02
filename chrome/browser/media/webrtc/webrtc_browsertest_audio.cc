// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/webrtc_browsertest_audio.h"

#include <stddef.h>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "media/audio/sounds/wav_audio_handler.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_parameters.h"

namespace {
// Opens |wav_filename|, reads it and loads it as a wav file. This function will
// bluntly trigger CHECKs if we can't read the file or if it's malformed. The
// caller takes ownership of the returned data. The size of the data is stored
// in |read_length|.
std::unique_ptr<char[]> ReadWavFile(const base::FilePath& wav_filename,
                                    size_t* file_length) {
  base::File wav_file(
      wav_filename, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!wav_file.IsValid()) {
    CHECK(false) << "Failed to read " << wav_filename.value();
    return nullptr;
  }

  size_t wav_file_length = wav_file.GetLength();

  std::unique_ptr<char[]> data(new char[wav_file_length]);
  size_t read_bytes = wav_file.Read(0, data.get(), wav_file_length);
  if (read_bytes != wav_file_length) {
    LOG(ERROR) << "Failed to read all bytes of " << wav_filename.value();
    return nullptr;
  }
  *file_length = wav_file_length;
  return data;
}
}  // namespace

namespace test {

}  // namespace test
