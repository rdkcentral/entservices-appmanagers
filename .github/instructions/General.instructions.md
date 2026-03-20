---
applyTo: "**/*Implementation.cpp,**/*Implementation.h,**/*.cpp,**/*.h"
---

# Instruction Summary
  1. [Critical Logging](https://github.com/rdkcentral/entservices-appmanagers/blob/develop/.github/instructions/General.instructions.md#critical-logging)
  2. [Recoverable Error Reporting](https://github.com/rdkcentral/entservices-appmanagers/blob/develop/.github/instructions/General.instructions.md#recoverable-error-reporting)
  3. [YODA Notation in conditionals](https://github.com/rdkcentral/entservices-appmanagers/blob/develop/.github/instructions/General.instructions.md#yoda-notation-in-conditionals)
  4. [Avoiding Public Member Variables](https://github.com/rdkcentral/entservices-appmanagers/blob/develop/.github/instructions/General.instructions.md#avoiding-public-member-variables)
  5. [Using nullptr instead of NULL](https://github.com/rdkcentral/entservices-appmanagers/blob/develop/.github/instructions/General.instructions.md#using-nullptr-instead-of-null)
  6. [C++ Naming Conventions](https://github.com/rdkcentral/entservices-appmanagers/blob/develop/.github/instructions/General.instructions.md#c-naming-conventions)

### Critical Logging
### Requirement

All the critical errors in the plugin must be logged using the Thunder Framework logging mechanism. Use the following macro to log critical errors:

```
LOGERR("Error Message with parameters: %d, %s", intParam, strParam.c_str());
```

If the function returns an error code, ensure to return the appropriate Core::ERROR_* code after logging the error. Success codes should not be logged to prevent log cluttering. If a given PR is a Hotfix, ensure that the logging is essential to avoid performance degradation. 

### Example

```cpp
if (nullptr == pointer) {
    LOGERR("Pointer is null in function %s", __FUNCTION__);
    return Core::ERROR_BAD_REQUEST;
}
```

### Incorrect Example

```cpp
if (nullptr == pointer) {
    return Core::ERROR_BAD_REQUEST;
}
```


### Recoverable Error Reporting
### Requirement

Do not log recoverable conditions as errors; use LOGWARN instead of LOGERR. Use the following macro to log warning messages.

```
LOGWARN("Warning Message with parameters: %d, %s", intParam, strParam.c_str());
``` 

### Example

```cpp
// optional data is missing
if (nullptr == optionalData) {
    LOGWARN("Optional data is missing in function %s", __FUNCTION__);
} // proceed with default behavior
```

### Incorrect Example

```cpp
// optional data is missing
if (nullptr == optionalData) {
    LOGERR("Optional data is missing in function %s", __FUNCTION__);
} // proceed with default behavior
```


### YODA Notation in conditionals
### Requirement
When writing conditionals, use YODA notation to prevent accidental assignment errors. This means placing constants on the left side of the comparison.
### Example

```cpp
if (nullptr == pointer) {
    LOGERR("Pointer is null in function %s", __FUNCTION__);
    return Core::ERROR_BAD_REQUEST;
}
```

### Incorrect Example

```cpp
if (pointer == nullptr) {
    LOGERR("Pointer is null in function %s", __FUNCTION__);
    return Core::ERROR_BAD_REQUEST;
}
```


### Avoiding Public Member Variables
### Requirement
Avoid using public member variables in classes. Instead, use private member variables with public getter and setter methods to encapsulate the data.
### Example

```cpp
class MyClass {
public:
    int GetValue() const { return value; }
    void SetValue(int val) { value = val; }

private:
    int value;
};
```

### Incorrect Example

```cpp
class MyClass {
public:
    int value; // Public member variable
};
```


### Using nullptr instead of NULL
### Requirement
Use `nullptr` instead of `NULL` for pointer initialization and comparisons to improve type safety and clarity.
### Example 

```cpp
int* ptr = nullptr;
if (nullptr == ptr) {
    // Handle null pointer
}
```

### Incorrect Example

```cpp
int* ptr = NULL;
if (NULL == ptr) {
    // Handle null pointer
}
```


### C++ Naming Conventions
### Requirement
Follow these naming conventions for C++ identifiers to maintain consistency and readability across the codebase:

- **Class names and types**: Use `PascalCase` (e.g., `MyClass`, `PluginManager`)
- **Class member variables**: Prefix with `m` and use `camelCase` (e.g., `mPluginCount`, `mIsInitialized`)
- **Static member variables**: Prefix with `s` and use `camelCase` (e.g., `sMaxRetries`, `sDefaultTimeout`)
- **Global variables**: Prefix with `g` and use `camelCase` (e.g., `gPluginRegistry`, `gDefaultConfig`)
- **Local variables**: Use `camelCase` (e.g., `localCounter`, `resultCode`)
- **Function parameters**: Use `camelCase` for all function parameters, including class member functions (e.g., `pluginId`, `configPath`, `newValue`)
- **Member functions**: Use `PascalCase` (e.g., `InitializePlugin()`, `GetStatus()`, `SetValue()`)
- **Constant names**: Use `UPPER_SNAKE_CASE` (e.g., `MAX_BUFFER_SIZE`, `DEFAULT_TIMEOUT`)

### Example

```cpp
class PluginManager {
private:
    int mPluginCount;              // Member variable
    static int sMaxPlugins;        // Static member variable
    
public:
    void RegisterPlugin(int pluginId) {     // Function parameter: camelCase
        int localCounter = 0;      // Local variable: camelCase
        mPluginCount++;
    }
    
    int GetPluginCount() const {   // Member function: PascalCase
        return mPluginCount;
    }
    
    void SetValue(int newValue) {  // Setter with parameter: camelCase
        mPluginCount = newValue;
    }
};

int PluginManager::sMaxPlugins = 100;      // Static member variable
int gGlobalPluginId = 0;                   // Global variable: g prefix + camelCase
const int MAX_RETRY_COUNT = 3;             // Constant: UPPER_SNAKE_CASE
```

### Incorrect Example

```cpp
class PluginManager {              // WRONG: pluginManager (should be PascalCase)
private:
    int pluginCount;               // WRONG: missing 'm' prefix (should be mPluginCount)
    static int maxPlugins;         // WRONG: missing 's' prefix (should be sMaxPlugins)
    
public:
    void registerPlugin(int PluginId) {     // WRONG: should be RegisterPlugin with camelCase param pluginId
        int LocalCounter = 0;      // WRONG: should be localCounter
        pluginCount++;
    }
    
    int getPluginCount() const {   // WRONG: should be GetPluginCount (PascalCase)
        return pluginCount;
    }
    
    void setvalue(int value) {     // WRONG: should be SetValue
        pluginCount = value;
    }
};

int maxPlugins = 100;              // WRONG: should be sMaxPlugins for static or gMaxPlugins for global
int global_plugin_id = 0;          // WRONG: should be gGlobalPluginId (g prefix + camelCase)
const int max_retry_count = 3;     // WRONG: should be MAX_RETRY_COUNT (UPPER_SNAKE_CASE)
```

