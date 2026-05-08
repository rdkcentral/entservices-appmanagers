#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace L0Test {
namespace RDKWMShim {

void Reset();

void SetCreateDisplayResult(bool value);
void SetGetClientsResult(bool value, const std::vector<std::string>& clients);
void SetSetFocusResult(bool value);
void SetVisibilityResult(bool setVisibleResult, bool getVisibleResult, bool visibleValue);
void SetRenderReadyResult(bool value);
void SetEnableDisplayRenderResult(bool value);
void SetLastKeyResult(bool value, uint32_t keyCode, uint32_t modifiers, uint64_t timestamp);
void SetZOrderResult(bool setResult, bool getResult, int32_t zOrder);
void SetVncResults(bool startResult, bool stopResult);
void SetKeyRepeatResult(bool enableResult, bool enabledValue);
void SetGetKeyRepeatsEnabledResult(bool value);
void SetInputEventsResult(bool value);
void SetKeyOpsResult(bool addInterceptResult, bool removeInterceptResult, bool addListenerResult, bool removeListenerResult, bool injectResult, bool generateResult);
void SetScreenshotResult(bool result, const std::string& imageData);

void EmitOnConnected(const std::string& client);
void EmitOnDisconnected(const std::string& client);
void EmitOnReady(const std::string& client);
void EmitOnVisible(const std::string& client);
void EmitOnHidden(const std::string& client);
void EmitOnFocus(const std::string& client);
void EmitOnBlur(const std::string& client);
void EmitOnInactive(double minutes);

} // namespace RDKWMShim
} // namespace L0Test
