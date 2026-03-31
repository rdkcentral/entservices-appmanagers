# RDKWindowManager L1 Test Coverage

## Scope

- Plugin under test: `RDKWindowManager/`
- Interface reference: `ref/IRDKWindowManager.h`
- L1 test file: `Tests/L1Tests/tests/test_RDKWindowManager.cpp`
- Test fixture: `RDKWindowManagerTest`
- Total test cases: **70**

---

## Test Infrastructure

### Fixture Setup (`SetUp`)

| Step | Description |
|---|---|
| Plugin creation | `Core::ProxyType<Plugin::RDKWindowManager>::Create()` — creates the WPEFramework plugin proxy |
| Worker pool | `WorkerPoolImplementation(2 threads)` assigned to `Core::IWorkerPool` |
| Mock injection | `NiceMock<COMLinkMock>` provided via `ServiceMock::COMLink()` |
| Implementation mock | `NiceMock<WindowManagerMock>` returned by `COMLink()->Instantiate()`, replacing `RDKWindowManagerImplementation` entirely |
| Initialization | `plugin->Initialize(&serviceMock)` — expects `WindowManagerMock::Initialize` and `WindowManagerMock::Register` to return `Core::ERROR_NONE` |
| JSONRPC handler | `Core::JSONRPC::Handler& handler` bound to the plugin for all `handler.Invoke()` calls |

### Fixture Teardown (`TearDown`)

| Step | Description |
|---|---|
| Deinitialize | `plugin->Deinitialize(&serviceMock)` — expects `WindowManagerMock::Unregister` and `WindowManagerMock::Deinitialize` |
| Dispatcher | `dispatcher->Deactivate()` / `dispatcher->Release()` |
| Factory reset | `PluginHost::IFactories::Assign(nullptr)` |
| Mock cleanup | `delete windowManagerMock` |

### Dependencies Mocked

| Mock | Header | Purpose |
|---|---|---|
| `WindowManagerMock` | `Tests/mocks/WindowManagerMock.h` | Full `IRDKWindowManager` COM-RPC interface mock |
| `ServiceMock` | `Tests/mocks/ServiceMock.h` | `PluginHost::IShell` mock |
| `COMLinkMock` | `Tests/mocks/COMLinkMock.h` | `IShell::ICOMLink` mock — delivers `WindowManagerMock` via `Instantiate()` |
| `FactoriesImplementation` | `Tests/mocks/FactoriesImplementation.h` | `PluginHost::IFactories` for JSONRPC message allocation |
| `WorkerPoolImplementation` | `Tests/mocks/WorkerPoolImplementation.h` | `Core::IWorkerPool` for Thunder job dispatch |

---

## API Coverage

### Registered Methods (`RegisteredMethodsExist`)

Verifies that all 28 public JSON-RPC method names returned by `handler.Exists()` match the `@text` annotations in `IRDKWindowManager.h`.

| Method name verified |
|---|
| `createDisplay` |
| `getApps` |
| `addKeyIntercept` |
| `addKeyIntercepts` |
| `removeKeyIntercept` |
| `addKeyListener` |
| `removeKeyListener` |
| `injectKey` |
| `generateKey` |
| `enableInactivityReporting` |
| `setInactivityInterval` |
| `resetInactivityTime` |
| `enableKeyRepeats` |
| `getKeyRepeatsEnabled` |
| `ignoreKeyInputs` |
| `enableInputEvents` |
| `keyRepeatConfig` |
| `setFocus` |
| `setVisible` |
| `getVisibility` |
| `renderReady` |
| `enableDisplayRender` |
| `getLastKeyInfo` |
| `setZOrder` |
| `getZOrder` |
| `startVncServer` |
| `stopVncServer` |
| `getScreenshot` |

---

### Detailed Test Case Inventory

#### `createDisplay`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `CreateDisplay_Success` | Valid display params with client, name, width, height | `CreateDisplay(_)` → `ERROR_NONE` | `ERROR_NONE` |
| `CreateDisplay_Failure` | Empty client / display name | `CreateDisplay(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `getApps`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `GetApps_Success` | Active apps available; output string populated | `GetApps(_)` sets `appsIds` → `ERROR_NONE` | `ERROR_NONE` |
| `GetApps_Failure` | Backend error retrieving app list | `GetApps(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `addKeyIntercept`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `AddKeyIntercept_Success` | Valid client, keyCode 65, ctrl modifier | `AddKeyIntercept(_)` → `ERROR_NONE` | `ERROR_NONE` |
| `AddKeyIntercept_Failure` | Empty client / keyCode 0 | `AddKeyIntercept(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `addKeyIntercepts`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `AddKeyIntercepts_Success` | Valid clientId with intercept array | `AddKeyIntercepts("testApp", _)` → `ERROR_NONE` | `ERROR_NONE` |
| `AddKeyIntercepts_Failure` | Empty intercepts array | `AddKeyIntercepts(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `removeKeyIntercept`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `RemoveKeyIntercept_Success` | clientId + keyCode 65 + ctrl modifier | `RemoveKeyIntercept("testApp", 65, _)` → `ERROR_NONE` | `ERROR_NONE` |
| `RemoveKeyIntercept_Failure` | Backend error removing intercept | `RemoveKeyIntercept(_, _, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `addKeyListener`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `AddKeyListener_Success` | Valid key listener with client and key config | `AddKeyListener(_)` → `ERROR_NONE` | `ERROR_NONE` |
| `AddKeyListener_Failure` | Empty key listeners object | `AddKeyListener(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `removeKeyListener`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `RemoveKeyListener_Success` | Valid key listener removal | `RemoveKeyListener(_)` → `ERROR_NONE` | `ERROR_NONE` |
| `RemoveKeyListener_Failure` | Backend error removing listener | `RemoveKeyListener(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `injectKey`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `InjectKey_Success` | keyCode 65, ctrl modifier | `InjectKey(65, _)` → `ERROR_NONE` | `ERROR_NONE` |
| `InjectKey_Failure` | keyCode 0, empty modifiers | `InjectKey(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `generateKey`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `GenerateKey_Success` | Keys array with client name | `GenerateKey(_, "testApp")` → `ERROR_NONE` | `ERROR_NONE` |
| `GenerateKey_Failure` | Empty keys array | `GenerateKey(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `enableInactivityReporting`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `EnableInactivityReporting_Enable_Success` | `enable = true` | `EnableInactivityReporting(true)` → `ERROR_NONE` | `ERROR_NONE` |
| `EnableInactivityReporting_Disable_Success` | `enable = false` | `EnableInactivityReporting(false)` → `ERROR_NONE` | `ERROR_NONE` |
| `EnableInactivityReporting_Failure` | Backend error | `EnableInactivityReporting(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `setInactivityInterval`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `SetInactivityInterval_Success` | Interval = 60 seconds | `SetInactivityInterval(60)` → `ERROR_NONE` | `ERROR_NONE` |
| `SetInactivityInterval_ZeroInterval_Success` | Boundary: interval = 0 | `SetInactivityInterval(0)` → `ERROR_NONE` | `ERROR_NONE` |
| `SetInactivityInterval_Failure` | Backend error | `SetInactivityInterval(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `resetInactivityTime`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `ResetInactivityTime_Success` | Normal reset | `ResetInactivityTime()` → `ERROR_NONE` | `ERROR_NONE` |
| `ResetInactivityTime_Failure` | Backend error | `ResetInactivityTime()` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `enableKeyRepeats`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `EnableKeyRepeats_Enable_Success` | `enable = true` | `EnableKeyRepeats(true)` → `ERROR_NONE` | `ERROR_NONE` |
| `EnableKeyRepeats_Disable_Success` | `enable = false` | `EnableKeyRepeats(false)` → `ERROR_NONE` | `ERROR_NONE` |
| `EnableKeyRepeats_Failure` | Backend error | `EnableKeyRepeats(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `getKeyRepeatsEnabled`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `GetKeyRepeatsEnabled_ReturnsTrue_Success` | Key repeats enabled | `GetKeyRepeatsEnabled(_)` sets `keyRepeat=true` → `ERROR_NONE` | `ERROR_NONE`; response JSON `keyRepeat=true` verified |
| `GetKeyRepeatsEnabled_ReturnsFalse_Success` | Key repeats disabled | `GetKeyRepeatsEnabled(_)` sets `keyRepeat=false` → `ERROR_NONE` | `ERROR_NONE`; response JSON `keyRepeat=false` verified |
| `GetKeyRepeatsEnabled_Failure` | Backend error | `GetKeyRepeatsEnabled(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `ignoreKeyInputs`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `IgnoreKeyInputs_Ignore_Success` | `ignore = true` | `IgnoreKeyInputs(true)` → `ERROR_NONE` | `ERROR_NONE` |
| `IgnoreKeyInputs_Unignore_Success` | `ignore = false` | `IgnoreKeyInputs(false)` → `ERROR_NONE` | `ERROR_NONE` |
| `IgnoreKeyInputs_Failure` | Backend error | `IgnoreKeyInputs(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `enableInputEvents`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `EnableInputEvents_Enable_Success` | Multiple clients, `enable = true` | `EnableInputEvents(_, true)` → `ERROR_NONE` | `ERROR_NONE` |
| `EnableInputEvents_Disable_Success` | Single client, `enable = false` | `EnableInputEvents(_, false)` → `ERROR_NONE` | `ERROR_NONE` |
| `EnableInputEvents_Failure` | Empty clients, backend error | `EnableInputEvents(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `keyRepeatConfig`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `KeyRepeatConfig_Default_Success` | Input type `"default"`, enabled + intervals | `KeyRepeatConfig("default", _)` → `ERROR_NONE` | `ERROR_NONE` |
| `KeyRepeatConfig_Keyboard_Success` | Input type `"keyboard"`, enabled + intervals | `KeyRepeatConfig("keyboard", _)` → `ERROR_NONE` | `ERROR_NONE` |
| `KeyRepeatConfig_Failure` | Backend error | `KeyRepeatConfig(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `setFocus`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `SetFocus_Success` | Valid client `"testApp"` | `SetFocus("testApp")` → `ERROR_NONE` | `ERROR_NONE` |
| `SetFocus_Failure_ClientNotFound` | Non-existent client | `SetFocus(_)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `setVisible`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `SetVisible_Show_Success` | Show client (`visible = true`) | `SetVisible("testApp", true)` → `ERROR_NONE` | `ERROR_NONE` |
| `SetVisible_Hide_Success` | Hide client (`visible = false`) | `SetVisible("testApp", false)` → `ERROR_NONE` | `ERROR_NONE` |
| `SetVisible_Failure` | Backend error | `SetVisible(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `getVisibility`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `GetVisibility_Visible_Success` | Client is visible | `GetVisibility("testApp", _)` sets `visible=true` → `ERROR_NONE` | `ERROR_NONE`; response JSON `visible=true` verified |
| `GetVisibility_Hidden_Success` | Client is hidden | `GetVisibility("testApp", _)` sets `visible=false` → `ERROR_NONE` | `ERROR_NONE`; response JSON `visible=false` verified |
| `GetVisibility_Failure` | Backend error | `GetVisibility(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `renderReady`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `RenderReady_Ready_Success` | First frame rendered | `RenderReady("testApp", _)` sets `status=true` → `ERROR_NONE` | `ERROR_NONE`; response JSON `status=true` verified |
| `RenderReady_NotReady_Success` | First frame not yet rendered | `RenderReady("testApp", _)` sets `status=false` → `ERROR_NONE` | `ERROR_NONE`; response JSON `status=false` verified |
| `RenderReady_Failure` | Backend error | `RenderReady(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `enableDisplayRender`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `EnableDisplayRender_Enable_Success` | Enable Wayland display render | `EnableDisplayRender("testApp", true)` → `ERROR_NONE` | `ERROR_NONE` |
| `EnableDisplayRender_Disable_Success` | Disable Wayland display render | `EnableDisplayRender("testApp", false)` → `ERROR_NONE` | `ERROR_NONE` |
| `EnableDisplayRender_Failure` | Backend error | `EnableDisplayRender(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `getLastKeyInfo`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `GetLastKeyInfo_Success` | Key press available | `GetLastKeyInfo(_, _, _)` sets keyCode=65, modifiers=1, timestamp → `ERROR_NONE` | `ERROR_NONE`; response JSON `keyCode` and `modifiers` values verified |
| `GetLastKeyInfo_NoKeyPressAvailable_Failure` | No key press recorded | `GetLastKeyInfo(_, _, _)` → `ERROR_UNAVAILABLE` | `ERROR_UNAVAILABLE` |
| `GetLastKeyInfo_GeneralFailure` | Backend error | `GetLastKeyInfo(_, _, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `setZOrder`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `SetZOrder_Success` | Positive z-order value (5) | `SetZOrder("testAppInstance", 5)` → `ERROR_NONE` | `ERROR_NONE` |
| `SetZOrder_NegativeZOrder_Success` | Boundary: negative z-order (-1) | `SetZOrder("testAppInstance", -1)` → `ERROR_NONE` | `ERROR_NONE` |
| `SetZOrder_Failure` | Backend error | `SetZOrder(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `getZOrder`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `GetZOrder_Success` | Z-order value available | `GetZOrder("testAppInstance", _)` sets `zOrder=3` → `ERROR_NONE` | `ERROR_NONE`; response JSON `zOrder=3` verified |
| `GetZOrder_Failure` | Backend error | `GetZOrder(_, _)` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `startVncServer`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `StartVncServer_Success` | VNC server starts successfully | `StartVncServer()` → `ERROR_NONE` | `ERROR_NONE` |
| `StartVncServer_Failure` | VNC server fails to start | `StartVncServer()` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `stopVncServer`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `StopVncServer_Success` | VNC server stops successfully | `StopVncServer()` → `ERROR_NONE` | `ERROR_NONE` |
| `StopVncServer_Failure` | VNC server fails to stop | `StopVncServer()` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

#### `getScreenshot`

| Test case | Scenario | Mock expectation | Expected result |
|---|---|---|---|
| `GetScreenshot_Success` | Screenshot capture initiated | `GetScreenshot()` → `ERROR_NONE` | `ERROR_NONE` |
| `GetScreenshot_Failure` | Screenshot capture fails | `GetScreenshot()` → `ERROR_GENERAL` | `ERROR_GENERAL` |

---

## Summary Table

| API (`@text` name) | Total tests | Success paths | Failure paths | Output validation |
|---|:---:|:---:|:---:|:---:|
| `createDisplay` | 2 | 1 | 1 | — |
| `getApps` | 2 | 1 | 1 | — |
| `addKeyIntercept` | 2 | 1 | 1 | — |
| `addKeyIntercepts` | 2 | 1 | 1 | — |
| `removeKeyIntercept` | 2 | 1 | 1 | — |
| `addKeyListener` | 2 | 1 | 1 | — |
| `removeKeyListener` | 2 | 1 | 1 | — |
| `injectKey` | 2 | 1 | 1 | — |
| `generateKey` | 2 | 1 | 1 | — |
| `enableInactivityReporting` | 3 | 2 | 1 | — |
| `setInactivityInterval` | 3 | 2 | 1 | — |
| `resetInactivityTime` | 2 | 1 | 1 | — |
| `enableKeyRepeats` | 3 | 2 | 1 | — |
| `getKeyRepeatsEnabled` | 3 | 2 | 1 | `keyRepeat` (bool) |
| `ignoreKeyInputs` | 3 | 2 | 1 | — |
| `enableInputEvents` | 3 | 2 | 1 | — |
| `keyRepeatConfig` | 3 | 2 | 1 | — |
| `setFocus` | 2 | 1 | 1 | — |
| `setVisible` | 3 | 2 | 1 | — |
| `getVisibility` | 3 | 2 | 1 | `visible` (bool) |
| `renderReady` | 3 | 2 | 1 | `status` (bool) |
| `enableDisplayRender` | 3 | 2 | 1 | — |
| `getLastKeyInfo` | 3 | 1 | 2 | `keyCode`, `modifiers` (uint32) |
| `setZOrder` | 3 | 2 | 1 | — |
| `getZOrder` | 2 | 1 | 1 | `zOrder` (int32) |
| `startVncServer` | 2 | 1 | 1 | — |
| `stopVncServer` | 2 | 1 | 1 | — |
| `getScreenshot` | 2 | 1 | 1 | — |
| **Registration** | 1 | 1 | — | 28 method names |
| **Total** | **70** | **42** | **27** | |

---

## APIs Not Covered by L1 Tests

The following interface methods are excluded from L1 test scope per the test generation requirements:

| Method | Reason |
|---|---|
| `Register(INotification*)` | Notification/event infrastructure — excluded per requirements |
| `Unregister(INotification*)` | Notification/event infrastructure — excluded per requirements |
| `Initialize(IShell*)` | Plugin lifecycle, not a public JSON-RPC method (`@json:omit`) |
| `Deinitialize(IShell*)` | Plugin lifecycle, not a public JSON-RPC method (`@json:omit`) |

Event callbacks (`OnUserInactivity`, `OnDisconnected`, `OnReady`, `OnConnected`, `OnVisible`, `OnHidden`, `OnFocus`, `OnBlur`, `OnScreenshotComplete`) are excluded per the "no event tests" requirement.
