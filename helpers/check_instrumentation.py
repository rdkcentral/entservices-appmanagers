#!/usr/bin/env python3
"""
Quick script to verify timing instrumentation has been applied to all plugins
"""

import os
import sys
from pathlib import Path

PLUGINS = [
    "AppManager",
    "LifecycleManager", 
    "RuntimeManager",
    "PackageManager",
    "DownloadManager",
    "PreinstallManager",
    "StorageManager",
    "TelemetryMetrics"
]

def check_instrumentation(base_path):
    """Check if timing instrumentation is present in all plugins"""
    results = {}
    
    for plugin in PLUGINS:
        impl_file = Path(base_path) / plugin / f"{plugin}Implementation.cpp"
        
        if not impl_file.exists():
            results[plugin] = {"status": "FILE_NOT_FOUND", "count": 0}
            continue
            
        with open(impl_file, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            
        # Check for header include
        has_include = '#include "RDKAppManagersDebugTime.h"' in content
        
        # Count RDKAPPMANAGERS_DEBUG_TIME_SCOPE usages
        scope_count = content.count('RDKAPPMANAGERS_DEBUG_TIME_SCOPE')
        
        results[plugin] = {
            "status": "INSTRUMENTED" if has_include and scope_count > 0 else "MISSING",
            "has_include": has_include,
            "scope_count": scope_count
        }
    
    return results

def main():
    if len(sys.argv) > 1:
        base_path = sys.argv[1]
    else:
        # Try to detect base path
        script_dir = Path(__file__).parent
        base_path = script_dir.parent
    
    print(f"Checking instrumentation in: {base_path}\n")
    
    results = check_instrumentation(base_path)
    
    print("=" * 70)
    print("TIMING INSTRUMENTATION STATUS")
    print("=" * 70)
    print(f"{'Plugin':<25} {'Status':<15} {'Include':<10} {'Scopes':<10}")
    print("-" * 70)
    
    total_scopes = 0
    all_good = True
    
    for plugin in PLUGINS:
        result = results[plugin]
        status = result['status']
        has_include = "✓" if result.get('has_include') else "✗"
        scope_count = result.get('scope_count', 0)
        total_scopes += scope_count
        
        status_icon = "✓" if status == "INSTRUMENTED" else "✗"
        print(f"{plugin:<25} {status_icon} {status:<13} {has_include:<10} {scope_count:<10}")
        
        if status != "INSTRUMENTED":
            all_good = False
    
    print("-" * 70)
    print(f"Total instrumented scopes: {total_scopes}")
    print()
    
    if all_good:
        print("✓ All plugins are properly instrumented!")
        return 0
    else:
        print("✗ Some plugins are missing instrumentation.")
        print("  Run this script from the project root directory.")
        return 1

if __name__ == '__main__':
    sys.exit(main())
