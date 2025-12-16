#include "SPF/Modules/IBindableInput.hpp"

SPF_NS_BEGIN
namespace Modules {
// Base class has no implementation for most methods,
// but we can provide a default for IsSameAs.
bool IBindableInput::IsSameAs(const IBindableInput& other) const {
  // By default, we consider two inputs the same if their JSON representations are identical.
  // This is a robust fallback for any input type.
  return this->ToJson() == other.ToJson();
}
}  // namespace Modules
SPF_NS_END