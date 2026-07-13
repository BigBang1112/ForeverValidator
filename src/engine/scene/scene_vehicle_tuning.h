#pragma once

#include <functional>
#include <optional>

#include "engine/core/cmw_nod.h"
#include "engine/core/func_keys_real.h"
class CSceneVehicleTuningCurve {
public:
  CSceneVehicleTuningCurve(void) = default;

  void Bind(CFuncKeysReal &curve) { curve_ = std::ref(curve); }
  void Reset(void) { curve_.reset(); }
  bool IsBound(void) const { return curve_.has_value(); }
  CFuncKeysReal &Value(void) const { return curve_->get(); }

private:
  std::optional<std::reference_wrapper<CFuncKeysReal>> curve_;
};

class CSceneVehicleTuning : public CMwNod {
public:
  CSceneVehicleTuning(void);
  ~CSceneVehicleTuning(void) override;
  virtual void CreateDefaultData(void);
};
