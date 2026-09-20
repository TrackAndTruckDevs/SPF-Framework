#pragma once

namespace SPF::Telemetry::SCS {

// Represents a set of control inputs (e.g., from a device or as used by the game)
struct ControlValues {
  float steering = 0.0f;
  float throttle = 0.0f;
  float brake = 0.0f;
  float clutch = 0.0f;
};

// Groups user input and the final effective values used by the simulation
struct Controls {
  ControlValues userInput;       // Raw input from the user's devices
  ControlValues effectiveInput;  // Input after game processing (e.g., speed-based steering)
};
}  // namespace SPF::Telemetry::SCS
