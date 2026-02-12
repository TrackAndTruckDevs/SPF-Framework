#include "SPF/Modules/ChordInput.hpp"
#include <set>
#include <sstream>
#include <algorithm>

SPF_NS_BEGIN
namespace Modules {

ChordInput::ChordInput() = default;
ChordInput::~ChordInput() = default;

void ChordInput::AddInput(std::unique_ptr<IBindableInput> input) {
    if (input && input->IsValid()) {
        m_inputs.push_back(std::move(input));
    }
}

std::vector<uint32_t> ChordInput::GetConstituentHardwareCodes() const {
    std::vector<uint32_t> codes;
    for (const auto& in : m_inputs) codes.push_back(in->GetHardwareCode());
    return codes;
}

bool ChordInput::IsTriggeredBy(const Input::KeyboardEvent& event) const {
    for (const auto& input : m_inputs) {
        if (input->IsTriggeredBy(event)) return true;
    }
    return false;
}

bool ChordInput::IsTriggeredBy(const Input::GamepadEvent& event) const {
    for (const auto& input : m_inputs) {
        if (input->IsTriggeredBy(event)) return true;
    }
    return false;
}

bool ChordInput::IsTriggeredBy(const Input::MouseButtonEvent& event) const {
    for (const auto& input : m_inputs) {
        if (input->IsTriggeredBy(event)) return true;
    }
    return false;
}

bool ChordInput::IsTriggeredBy(const Input::JoystickEvent& event) const {
    for (const auto& input : m_inputs) {
        if (input->IsTriggeredBy(event)) return true;
    }
    return false;
}

bool ChordInput::IsActive(const std::set<uint32_t>& pressedCodes) const {
    if (m_inputs.empty()) return false;
    for (const auto& input : m_inputs) {
        if (!input->IsActive(pressedCodes)) return false;
    }
    return true;
}

bool ChordInput::InvolvesHardwareCode(uint32_t code) const {
    for (const auto& input : m_inputs) {
        if (input->InvolvesHardwareCode(code)) return true;
    }
    return false;
}

bool ChordInput::IsSameAs(const IBindableInput& other) const {
    auto* otherChord = dynamic_cast<const ChordInput*>(&other);
    if (!otherChord) return false;
    
    if (m_inputs.size() != otherChord->m_inputs.size()) return false;

    // Use sets of hardware codes for order-independent comparison.
    // This ensures F1+F2 is the same as F2+F1.
    std::set<uint32_t> thisCodes;
    for (const auto& in : m_inputs) thisCodes.insert(in->GetHardwareCode());

    std::set<uint32_t> otherCodes;
    for (const auto& in : otherChord->m_inputs) otherCodes.insert(in->GetHardwareCode());

    return thisCodes == otherCodes;
}

nlohmann::ordered_json ChordInput::ToJson() const {
    nlohmann::ordered_json arr = nlohmann::ordered_json::array();
    for (const auto& input : m_inputs) {
        arr.push_back(input->ToJson());
    }
    return {{"type", "chord"}, {"bindings", arr}};
}

std::string ChordInput::GetDisplayName() const {
    std::stringstream ss;
    for (size_t i = 0; i < m_inputs.size(); ++i) {
        ss << m_inputs[i]->GetDisplayName();
        if (i < m_inputs.size() - 1) {
            ss << " + ";
        }
    }
    return ss.str();
}

bool ChordInput::IsValid() const {
    return !m_inputs.empty();
}

float ChordInput::GetValue(const std::set<uint32_t>& pressedHardwareCodes, const std::map<uint32_t, float>& axisValues) const {
    return IsActive(pressedHardwareCodes) ? 1.0f : 0.0f;
}

}  // namespace Modules
SPF_NS_END
