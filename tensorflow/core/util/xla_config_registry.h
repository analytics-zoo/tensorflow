/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_CORE_UTIL_XLA_CONFIG_REGISTRY_H
#define TENSORFLOW_CORE_UTIL_XLA_CONFIG_REGISTRY_H

#include <functional>
#include "tensorflow/core/platform/default/logging.h"
#include "tensorflow/core/platform/mutex.h"
#include "tensorflow/core/protobuf/config.pb.h"

namespace tensorflow {

// A registry class where XLA can register callbacks for Tensorflow to query
// its status.
class XlaConfigRegistry {
 public:
  // XlaGlobalJitLevel is used by XLA to expose its JIT level for processing
  // single gpu and general (multi-gpu) graphs.
  struct XlaGlobalJitLevel {
    OptimizerOptions::GlobalJitLevel single_gpu;
    OptimizerOptions::GlobalJitLevel general;
  };

  // Input is the jit_level in session config, and return value is the jit_level
  // from XLA, reflecting the effect of the environment variable flags.
  typedef std::function<XlaGlobalJitLevel(
      const OptimizerOptions::GlobalJitLevel&)>
      GlobalJitLevelGetterTy;

  static void Register(XlaConfigRegistry::GlobalJitLevelGetterTy getter) {
    static mutex mu(LINKER_INITIALIZED);
    mutex_lock l(mu);
    CHECK(!global_jit_level_getter_);
    global_jit_level_getter_ = std::move(getter);
  }

  static XlaGlobalJitLevel GetGlobalJitLevel(
      OptimizerOptions::GlobalJitLevel jit_level_in_session_opts) {
    if (!global_jit_level_getter_) {
      return {jit_level_in_session_opts, jit_level_in_session_opts};
    }
    return global_jit_level_getter_(jit_level_in_session_opts);
  }

 private:
  static GlobalJitLevelGetterTy global_jit_level_getter_;
};

#define REGISTER_XLA_CONFIG_GETTER(getter) \
  REGISTER_XLA_CONFIG_GETTER_UNIQ_HELPER(__COUNTER__, getter)

#define REGISTER_XLA_CONFIG_GETTER_UNIQ_HELPER(ctr, getter) \
  REGISTER_XLA_CONFIG_GETTER_UNIQ(ctr, getter)

#define REGISTER_XLA_CONFIG_GETTER_UNIQ(ctr, getter)   \
  static bool xla_config_registry_registration_##ctr = \
      (::tensorflow::XlaConfigRegistry::Register(getter), true)

}  // namespace tensorflow

#endif  // TENSORFLOW_CORE_UTIL_XLA_CONFIG_REGISTRY_H
