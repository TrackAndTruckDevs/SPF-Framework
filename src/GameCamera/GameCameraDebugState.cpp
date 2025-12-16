#include "SPF/GameCamera/GameCameraDebugState.hpp"
#include "SPF/GameCamera/GameCameraManager.hpp"
#include "SPF/GameCamera/GameCameraFree.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace GameCamera {
GameCameraDebugState::GameCameraDebugState() = default;

void GameCameraDebugState::SaveState() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  auto& camManager = GameCameraManager::GetInstance();

  // 1. Get the Free Camera object to read its current state
  auto* pFreeCam = camManager.GetCamera(GameCameraType::DeveloperFreeCamera);
  auto* freeCam = dynamic_cast<GameCameraFree*>(pFreeCam);

  if (!freeCam) {
    logger->Error("SaveState failed: GameCameraFree object not available.");
    return;
  }

  // 2. Gather all required data into the state snapshot struct
  CameraState state_snapshot = {};

  bool pos_ok = freeCam->GetPosition(&state_snapshot.pos_x, &state_snapshot.pos_y, &state_snapshot.pos_z);
  bool mystery_ok = freeCam->GetFreecamMysteryFloat(&state_snapshot.mystery_float);
  bool quat_ok = freeCam->GetQuaternion(&state_snapshot.q_x, &state_snapshot.q_y, &state_snapshot.q_z, &state_snapshot.q_w);
  bool fov_ok = freeCam->GetFov(&state_snapshot.fov);

  if (!pos_ok || !mystery_ok || !quat_ok || !fov_ok) {
    logger->Error("SaveState failed: Could not gather all required camera data.");
    return;
  }

  // 3. Execute the save sequence using the helper methods
  logger->Info("--- Initiating camera state save sequence ---");

  // Write to file
  uintptr_t fileHandle = OpenFile();
  if (fileHandle) {
    WriteStateToFile(fileHandle, state_snapshot);
    CloseFile(fileHandle);
  } else {
    logger->Error("Save sequence could not write to file: could not open file.");
  }

  // Add to in-memory array for state playback
  AddStateToMemory(state_snapshot);

  logger->Info("--- Camera state save sequence finished ---");
}

uintptr_t GameCameraDebugState::OpenFile() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();

  using OpenFileFunc = uintptr_t* (*)(uintptr_t*, const char**);
  auto pfnOpenFile = reinterpret_cast<OpenFileFunc>(dataService.GetOpenFileForCameraStateFunc());

  if (!pfnOpenFile) {
    logger->Error("Cannot open camera state file: OpenFileForCameraState function not found.");
    return 0;
  }

  uintptr_t file_handle = 0;
  const char* file_path = "/home/cams.txt";

  pfnOpenFile(&file_handle, &file_path);

  if (file_handle == 0) {
    logger->Error("Native OpenFileForCameraState function failed to return a file handle.");
  } else {
    logger->Info("Successfully opened camera state file, handle: {:#x}", file_handle);
  }

  return file_handle;
}

bool GameCameraDebugState::WriteStateToFile(uintptr_t fileHandle, const CameraState& state) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();

  using WriteStateFunc = void (*)(const CameraState*, uintptr_t);
  auto pfnWriteState = reinterpret_cast<WriteStateFunc>(dataService.GetFormatAndWriteCameraStateFunc());

  if (!pfnWriteState) {
    logger->Error("Cannot write camera state to file: FormatAndWriteCameraState function not found.");
    return false;
  }

  pfnWriteState(&state, fileHandle);
  logger->Info("Called native FormatAndWriteCameraState function.");

  return true;
}

void GameCameraDebugState::CloseFile(uintptr_t fileHandle) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  if (fileHandle == 0) {
    logger->Warn("CloseFile called with a null handle.");
    return;
  }

  // Based on the disassembly, the game calls a virtual function to close the file.
  // 1. Get the vtable pointer from the file object (handle).
  uintptr_t* vtable = *(uintptr_t**)fileHandle;
  if (vtable == nullptr) {
    logger->Error("Could not get vtable from file handle {:#x}", fileHandle);
    return;
  }

  // 2. Get the function pointer from the vtable (it's the first one).
  // The function signature is: void __fastcall(uintptr_t fileHandle, int flag);
  // The flag is 1 in the disassembly.
  using CloseFileFunc = void (*)(uintptr_t, int);
  CloseFileFunc pfnCloseFile = (CloseFileFunc)vtable[0];

  if (pfnCloseFile == nullptr) {
    logger->Error("Could not get CloseFile function pointer from vtable for handle {:#x}", fileHandle);
    return;
  }

  logger->Info("Calling native CloseFile function for handle {:#x}", fileHandle);
  // 3. Call the function.
  pfnCloseFile(fileHandle, 1);
}

void GameCameraDebugState::AddStateToMemory(const CameraState& state) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();

  // 1. Get the required native function and context pointer
  auto pfnAddState = (void (*)(void*, void*))dataService.GetAddCameraStateFunc();
  intptr_t contextOffset = dataService.GetStateContextOffset();
  uintptr_t pDebugCameraContext = dataService.GetDebugCameraContextPtr();

  if (!pfnAddState || contextOffset == 0 || !pDebugCameraContext) {
    logger->Error("Cannot add state to memory: required native function or offsets not found.");
    return;
  }

  uintptr_t pDebugCamera = *(uintptr_t*)(pDebugCameraContext);
  if (!pDebugCamera) {
    logger->Error("Cannot add state to memory: pDebugCamera object pointer is null.");
    return;
  }

  // 2. Log, calculate context, and call the native function
  void* pStateContext = (void*)(pDebugCamera + contextOffset);
  const void* pStateSnapshot = &state;

  logger->Info("Calling native AddCameraState with data:\n  Pos: ({:.2f}, {:.2f}, {:.2f})\n  Mystery: {:.4E}\n  Quat: ({:.2f}, {:.2f}, {:.2f}, {:.2f})\n  FOV: {:.2f}",
               state.pos_x,
               state.pos_y,
               state.pos_z,
               state.mystery_float,
               state.q_x,
               state.q_y,
               state.q_z,
               state.q_w,
               state.fov);

  pfnAddState(pStateContext, (void*)pStateSnapshot);

  logger->Info("Successfully called native AddCameraState function.");
}

void GameCameraDebugState::ApplyState(int index) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();

  auto pfnApplyState = reinterpret_cast<void (*)(void*, void*)>(dataService.GetApplyStateFunc());
  uintptr_t pDebugCameraContext = dataService.GetDebugCameraContextPtr();
  intptr_t indexOffset = dataService.GetStateCurrentIndexOffset();

  if (!pfnApplyState || !pDebugCameraContext || indexOffset == 0) {
    logger->Error("Cannot apply state: required native function or context/offset not found.");
    return;
  }

  uintptr_t stateAddress = GetStateAddress(index);
  if (!stateAddress) {
    logger->Error("Cannot apply state: failed to get address for index {}.", index);
    return;
  }

  uintptr_t pDebugCamera = *(uintptr_t*)(pDebugCameraContext);

  // 1. Call the native function to apply the visual state
  logger->Info("Calling native ApplyState function for index {}.", index);
  pfnApplyState((void*)pDebugCamera, (void*)stateAddress);

  // 2. Manually update the game's internal current index to match
  logger->Info("Manually updating current state index to {}.", index);
  *(uint64_t*)(pDebugCamera + indexOffset) = index;
}

void GameCameraDebugState::CycleState(int direction) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();

  // 1. Get the required native function and context pointer
  using CycleStateFunc = void (*)(void*, char);
  auto pfnCycleState = reinterpret_cast<CycleStateFunc>(dataService.GetCycleSavedStateFunc());
  uintptr_t pDebugCameraContext = dataService.GetDebugCameraContextPtr();

  if (!pfnCycleState || !pDebugCameraContext) {
    logger->Error("Cannot cycle camera state: required native function or context pointer not found.");
    return;
  }

  uintptr_t pDebugCamera = *(uintptr_t*)(pDebugCameraContext);
  if (!pDebugCamera) {
    logger->Error("Cannot cycle camera state: pDebugCamera object pointer is null.");
    return;
  }

  // 2. Call the native function
  char dir_char = static_cast<char>(direction);
  logger->Info("Calling native CycleSavedState function with direction: {}", direction);
  pfnCycleState((void*)pDebugCamera, dir_char);
}

int GameCameraDebugState::GetStateCount() const {
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pDebugCameraContext = dataService.GetDebugCameraContextPtr();
  intptr_t countOffset = dataService.GetStateCountOffset();

  if (!pDebugCameraContext || countOffset == 0) {
    return 0;
  }

  uintptr_t pDebugCamera = *(uintptr_t*)(pDebugCameraContext);
  if (!pDebugCamera) {
    return 0;
  }

  return static_cast<int>(*(uint64_t*)(pDebugCamera + countOffset));
}

bool GameCameraDebugState::GetState(int index, CameraState& out_state) const {
  uintptr_t stateAddress = GetStateAddress(index);
  if (!stateAddress) {
    return false;
  }

  // Copy the 36 bytes of data into our struct. The struct is larger due to padding,
  // so we only copy the data we know exists.
  memcpy(&out_state, (void*)stateAddress, NATIVE_STATE_SIZE);

  return true;
}

int GameCameraDebugState::GetCurrentStateIndex() const {
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pDebugCameraContext = dataService.GetDebugCameraContextPtr();
  intptr_t indexOffset = dataService.GetStateCurrentIndexOffset();

  if (!pDebugCameraContext || indexOffset == 0) {
    return -1;  // Return -1 to indicate an error or not found
  }

  uintptr_t pDebugCamera = *(uintptr_t*)(pDebugCameraContext);
  if (!pDebugCamera) {
    return -1;
  }

  return static_cast<int>(*(uint64_t*)(pDebugCamera + indexOffset));
}

uintptr_t GameCameraDebugState::GetStateAddress(int index) const {
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pDebugCameraContext = dataService.GetDebugCameraContextPtr();
  intptr_t arrayOffset = dataService.GetStateArrayOffset();
  int stateCount = GetStateCount();

  if (!pDebugCameraContext || arrayOffset == 0 || index < 0 || index >= stateCount) {
    return 0;
  }

  uintptr_t pDebugCamera = *(uintptr_t*)(pDebugCameraContext);
  if (!pDebugCamera) {
    return 0;
  }

  uintptr_t pStatesArray = *(uintptr_t*)(pDebugCamera + arrayOffset);
  if (!pStatesArray) {
    return 0;
  }

  return pStatesArray + (index * NATIVE_STATE_SIZE);
}

void GameCameraDebugState::ClearAllStatesInMemory() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();

  uintptr_t pDebugCameraContext = dataService.GetDebugCameraContextPtr();
  intptr_t countOffset = dataService.GetStateCountOffset();

  if (!pDebugCameraContext || countOffset == 0) {
    logger->Error("Cannot clear states: required context or offset not found.");
    return;
  }

  uintptr_t pDebugCamera = *(uintptr_t*)(pDebugCameraContext);
  if (!pDebugCamera) {
    logger->Error("Cannot clear states: pDebugCamera object pointer is null.");
    return;
  }

  logger->Info("Clearing all in-memory camera states.");
  *(uint64_t*)(pDebugCamera + countOffset) = 0;
}

bool GameCameraDebugState::EditStateInMemory(int index, const CameraState& newState) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  int stateCount = GetStateCount();

  if (index < 0 || index >= stateCount) {
    logger->Error("EditStateInMemory failed: index {} is out of bounds (count: {}).", index, stateCount);
    return false;
  }

  uintptr_t stateAddress = GetStateAddress(index);
  if (!stateAddress) {
    logger->Error("EditStateInMemory failed: could not get memory address for index {}.", index);
    return false;
  }

  logger->Info("Overwriting in-memory camera state at index {}.", index);
  memcpy((void*)stateAddress, &newState, NATIVE_STATE_SIZE);

  return true;
}

void GameCameraDebugState::ReloadStatesFromFile() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();

  // 1. Get all required pointers and offsets
  auto pfnLoadStates = reinterpret_cast<void (*)(void*)>(dataService.GetLoadStatesFromFileFunc());
  uintptr_t pDebugCameraContext = dataService.GetDebugCameraContextPtr();
  intptr_t countOffset = dataService.GetStateCountOffset();
  intptr_t indexOffset = dataService.GetStateCurrentIndexOffset();
  intptr_t managerOffset = dataService.GetStateManagerOffset();

  // 2. Validate everything
  if (!pfnLoadStates || !pDebugCameraContext || countOffset == 0 || indexOffset == 0 || managerOffset == 0) {
    logger->Error("Cannot reload states: required native function or offsets not found.");
    return;
  }

  uintptr_t pDebugCamera = *(uintptr_t*)(pDebugCameraContext);
  if (!pDebugCamera) {
    logger->Error("Cannot reload states: pDebugCamera object pointer is null.");
    return;
  }

  // 3. Reset the in-memory state count and index to 0 to allow reloading
  logger->Info("Resetting in-memory camera state count and index to 0.");
  *(uint64_t*)(pDebugCamera + countOffset) = 0;
  *(uint64_t*)(pDebugCamera + indexOffset) = 0;

  // 4. Call the native function to load the states from /home/cams.txt
  // The function expects a pointer to the state manager struct
  logger->Info("Calling native LoadStatesFromFile function.");
  pfnLoadStates((void*)(pDebugCamera + managerOffset));
}

void GameCameraDebugState::DeleteStateInMemory(int index) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugState");
  int stateCount = GetStateCount();

  if (index < 0 || index >= stateCount) {
    logger->Error("DeleteStateInMemory failed: index {} is out of bounds (count: {}).", index, stateCount);
    return;
  }

  // If we are deleting the very last element, we can just decrement the count.
  if (index == stateCount - 1) {
    logger->Info("Deleting last state (index {}). Simply decrementing count.", index);
  } else {
    // If not the last element, we need to shift the remaining elements to the left.
    uintptr_t pStatesArray = GetStateAddress(0);  // Get base address of the array
    if (!pStatesArray) {
      logger->Error("DeleteStateInMemory failed: could not get base address of states array.");
      return;
    }

    uintptr_t dest_addr = pStatesArray + (index * NATIVE_STATE_SIZE);
    uintptr_t src_addr = pStatesArray + ((index + 1) * NATIVE_STATE_SIZE);
    size_t num_elements_to_move = stateCount - 1 - index;
    size_t size_to_move = num_elements_to_move * NATIVE_STATE_SIZE;

    logger->Info("Deleting state at index {}. Shifting {} bytes from {:#x} to {:#x}.", index, size_to_move, src_addr, dest_addr);
    memmove((void*)dest_addr, (void*)src_addr, size_to_move);
  }

  // Finally, decrement the total state count.
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pDebugCameraContext = dataService.GetDebugCameraContextPtr();
  intptr_t countOffset = dataService.GetStateCountOffset();
  uintptr_t pDebugCamera = *(uintptr_t*)(pDebugCameraContext);

  if (pDebugCamera && countOffset) {
    *(uint64_t*)(pDebugCamera + countOffset) = stateCount - 1;
    logger->Info("State count decremented. New count: {}.", stateCount - 1);
  }
}
}  // namespace GameCamera
SPF_NS_END
