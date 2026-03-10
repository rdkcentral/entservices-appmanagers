#!/usr/bin/env python3
"""
RDK App Managers Timing Analyzer

This script analyzes timing logs from wpeframework.log to compute:
1. API execution time (entry to exit per API call)
2. Invoker → API immediate entry difference (how quickly the callee starts after the caller)
3. Plugin-to-plugin COM-RPC timing (cross-plugin call latencies)

Usage:
    python rdkappmanagers_time_analyzer.py wpeframework.log
    python rdkappmanagers_time_analyzer.py wpeframework.log --plugin-a AppManager --plugin-b LifecycleManager
    python rdkappmanagers_time_analyzer.py wpeframework.log --verbose
    python rdkappmanagers_time_analyzer.py wpeframework.log --output timing_report.txt
"""

import re
import sys
import argparse
from collections import defaultdict, namedtuple
from typing import Dict, List, Tuple, Optional
from datetime import datetime

# Named tuple for timing events
TimingEvent = namedtuple('TimingEvent', ['plugin', 'api', 'timestamp', 'event_type', 'duration', 'thread'])

# Regex pattern for RDKAPPMANAGERS_TIMING log entries
TIMING_PATTERN = re.compile(
    r'\[RDKAPPMANAGERS_TIMING\]\s+'
    r'(?P<event>ENTRY|EXIT)\s+'
    r'plugin=(?P<plugin>\w+)\s+'
    r'api=(?P<api>\w+)\s+'
    r'timestamp=(?P<timestamp>\d+)'
    r'(?:\s+duration_us=(?P<duration>\d+))?'
    r'(?:\s+thread=(?P<thread>0x[0-9a-fA-F]+))?'
)


class TimingAnalyzer:
    """Analyzes RDK App Manager timing logs"""
    
    def __init__(self):
        self.events: List[TimingEvent] = []
        self.api_calls: Dict[str, List[Tuple[int, int, int]]] = defaultdict(list)  # plugin:api -> [(start, end, duration)]
        self.thread_stacks: Dict[str, List[TimingEvent]] = defaultdict(list)  # thread_id -> stack of ENTRY events
        self.cross_plugin_calls: List[Dict] = []
        
    def parse_log_file(self, log_path: str) -> int:
        """Parse timing events from log file"""
        count = 0
        try:
            with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
                for line_num, line in enumerate(f, 1):
                    match = TIMING_PATTERN.search(line)
                    if match:
                        event = TimingEvent(
                            plugin=match.group('plugin'),
                            api=match.group('api'),
                            timestamp=int(match.group('timestamp')),
                            event_type=match.group('event'),
                            duration=int(match.group('duration')) if match.group('duration') else None,
                            thread=match.group('thread') if match.group('thread') else None
                        )
                        self.events.append(event)
                        count += 1
        except FileNotFoundError:
            print(f"Error: Log file not found: {log_path}")
            sys.exit(1)
        except Exception as e:
            print(f"Error parsing log file: {e}")
            sys.exit(1)
            
        return count
    
    def analyze_api_timings(self):
        """Analyze individual API execution times"""
        entry_stack = {}  # (plugin, api, thread) -> entry_event
        
        for event in self.events:
            key = (event.plugin, event.api, event.thread)
            
            if event.event_type == 'ENTRY':
                entry_stack[key] = event
            elif event.event_type == 'EXIT':
                if key in entry_stack:
                    entry_event = entry_stack[key]
                    duration = event.timestamp - entry_event.timestamp
                    api_key = f"{event.plugin}::{event.api}"
                    self.api_calls[api_key].append((entry_event.timestamp, event.timestamp, duration))
                    del entry_stack[key]
    
    def analyze_cross_plugin_calls(self):
        """Analyze cross-plugin call latencies"""
        # Build thread-based call stacks
        thread_stacks = defaultdict(list)
        
        for event in sorted(self.events, key=lambda e: e.timestamp):
            thread = event.thread or 'unknown'
            
            if event.event_type == 'ENTRY':
                # Check if there's a parent call from another plugin
                if thread_stacks[thread]:
                    parent = thread_stacks[thread][-1]
                    if parent.plugin != event.plugin:
                        # Cross-plugin call detected
                        call_start_delta = event.timestamp - parent.timestamp
                        self.cross_plugin_calls.append({
                            'caller_plugin': parent.plugin,
                            'caller_api': parent.api,
                            'callee_plugin': event.plugin,
                            'callee_api': event.api,
                            'caller_entry_ts': parent.timestamp,
                            'callee_entry_ts': event.timestamp,
                            'call_start_delta': call_start_delta,
                            'thread': thread
                        })
                
                thread_stacks[thread].append(event)
                
            elif event.event_type == 'EXIT':
                # Pop matching entry from stack
                if thread_stacks[thread]:
                    # Find matching entry
                    for i in range(len(thread_stacks[thread]) - 1, -1, -1):
                        if (thread_stacks[thread][i].plugin == event.plugin and
                            thread_stacks[thread][i].api == event.api):
                            entry = thread_stacks[thread].pop(i)
                            
                            # Update cross-plugin call with return delta
                            for call in reversed(self.cross_plugin_calls):
                                if (call['callee_plugin'] == event.plugin and
                                    call['callee_api'] == event.api and
                                    call['callee_entry_ts'] == entry.timestamp):
                                    call['callee_exit_ts'] = event.timestamp
                                    call['callee_duration'] = event.timestamp - entry.timestamp
                                    if 'caller_entry_ts' in call:
                                        call['return_delta'] = event.timestamp - call['caller_entry_ts']
                                    break
                            break
    
    def compute_statistics(self, values: List[int]) -> Dict:
        """Compute min, max, avg, median for a list of values"""
        if not values:
            return {'min': 0, 'max': 0, 'avg': 0, 'median': 0, 'count': 0}
        
        sorted_vals = sorted(values)
        count = len(sorted_vals)
        median = sorted_vals[count // 2] if count % 2 == 1 else (sorted_vals[count // 2 - 1] + sorted_vals[count // 2]) // 2
        
        return {
            'min': sorted_vals[0],
            'max': sorted_vals[-1],
            'avg': sum(sorted_vals) // count,
            'median': median,
            'count': count
        }
    
    def format_microseconds(self, us: int) -> str:
        """Format microseconds in human-readable format"""
        if us < 1000:
            return f"{us} µs"
        elif us < 1000000:
            return f"{us / 1000:.2f} ms"
        else:
            return f"{us / 1000000:.2f} s"
    
    def generate_report(self, plugin_a: Optional[str] = None, plugin_b: Optional[str] = None, 
                       verbose: bool = False) -> str:
        """Generate timing analysis report"""
        lines = []
        lines.append("=" * 80)
        lines.append("RDK APP MANAGERS TIMING ANALYSIS REPORT")
        lines.append("=" * 80)
        lines.append("")
        
        # Summary
        lines.append(f"Total timing events parsed: {len(self.events)}")
        lines.append(f"Unique APIs instrumented: {len(self.api_calls)}")
        lines.append(f"Cross-plugin calls detected: {len(self.cross_plugin_calls)}")
        lines.append("")
        
        # API Timing Statistics
        lines.append("-" * 80)
        lines.append("API EXECUTION TIME STATISTICS")
        lines.append("-" * 80)
        lines.append("")
        lines.append(f"{'Plugin::API':<40} {'Count':>8} {'Min':>12} {'Avg':>12} {'Max':>12} {'Median':>12}")
        lines.append("-" * 80)
        
        for api_key in sorted(self.api_calls.keys()):
            durations = [d for _, _, d in self.api_calls[api_key]]
            stats = self.compute_statistics(durations)
            
            lines.append(f"{api_key:<40} {stats['count']:>8} "
                        f"{self.format_microseconds(stats['min']):>12} "
                        f"{self.format_microseconds(stats['avg']):>12} "
                        f"{self.format_microseconds(stats['max']):>12} "
                        f"{self.format_microseconds(stats['median']):>12}")
        
        lines.append("")
        
        # Cross-Plugin Call Analysis
        if self.cross_plugin_calls:
            lines.append("-" * 80)
            lines.append("CROSS-PLUGIN CALL LATENCY ANALYSIS")
            lines.append("-" * 80)
            lines.append("")
            
            # Group by caller->callee pair
            call_pairs = defaultdict(list)
            for call in self.cross_plugin_calls:
                key = f"{call['caller_plugin']}::{call['caller_api']} → {call['callee_plugin']}::{call['callee_api']}"
                call_pairs[key].append(call)
            
            # Filter by plugin if specified
            if plugin_a or plugin_b:
                filtered_pairs = {}
                for key, calls in call_pairs.items():
                    sample = calls[0]
                    if plugin_a and plugin_b:
                        if (sample['caller_plugin'] == plugin_a and sample['callee_plugin'] == plugin_b) or \
                           (sample['caller_plugin'] == plugin_b and sample['callee_plugin'] == plugin_a):
                            filtered_pairs[key] = calls
                    elif plugin_a:
                        if sample['caller_plugin'] == plugin_a or sample['callee_plugin'] == plugin_a:
                            filtered_pairs[key] = calls
                    elif plugin_b:
                        if sample['caller_plugin'] == plugin_b or sample['callee_plugin'] == plugin_b:
                            filtered_pairs[key] = calls
                call_pairs = filtered_pairs
            
            if not call_pairs:
                lines.append("No cross-plugin calls found matching the filter criteria.")
            else:
                for pair_key in sorted(call_pairs.keys()):
                    calls = call_pairs[pair_key]
                    
                    call_start_deltas = [c['call_start_delta'] for c in calls]
                    return_deltas = [c['return_delta'] for c in calls if 'return_delta' in c]
                    callee_durations = [c['callee_duration'] for c in calls if 'callee_duration' in c]
                    
                    lines.append(f"\n{pair_key}")
                    lines.append(f"  Invocations: {len(calls)}")
                    
                    if call_start_deltas:
                        stats = self.compute_statistics(call_start_deltas)
                        lines.append(f"  Call Start Delta (caller ENTRY → callee ENTRY):")
                        lines.append(f"    Min: {self.format_microseconds(stats['min'])}, "
                                   f"Avg: {self.format_microseconds(stats['avg'])}, "
                                   f"Max: {self.format_microseconds(stats['max'])}, "
                                   f"Median: {self.format_microseconds(stats['median'])}")
                    
                    if callee_durations:
                        stats = self.compute_statistics(callee_durations)
                        lines.append(f"  Callee Execution Time:")
                        lines.append(f"    Min: {self.format_microseconds(stats['min'])}, "
                                   f"Avg: {self.format_microseconds(stats['avg'])}, "
                                   f"Max: {self.format_microseconds(stats['max'])}, "
                                   f"Median: {self.format_microseconds(stats['median'])}")
                    
                    if return_deltas:
                        stats = self.compute_statistics(return_deltas)
                        lines.append(f"  Return Delta (caller ENTRY → callee EXIT):")
                        lines.append(f"    Min: {self.format_microseconds(stats['min'])}, "
                                   f"Avg: {self.format_microseconds(stats['avg'])}, "
                                   f"Max: {self.format_microseconds(stats['max'])}, "
                                   f"Median: {self.format_microseconds(stats['median'])}")
                    
                    if verbose and len(calls) <= 10:
                        lines.append(f"  Individual Calls:")
                        for i, call in enumerate(calls[:10], 1):
                            lines.append(f"    #{i}: call_start_delta={self.format_microseconds(call['call_start_delta'])}")
                            if 'callee_duration' in call:
                                lines.append(f"        callee_duration={self.format_microseconds(call['callee_duration'])}")
                            if 'return_delta' in call:
                                lines.append(f"        return_delta={self.format_microseconds(call['return_delta'])}")
        
        lines.append("")
        lines.append("=" * 80)
        lines.append("END OF REPORT")
        lines.append("=" * 80)
        
        return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description='Analyze RDK App Manager timing logs',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s wpeframework.log
  %(prog)s wpeframework.log --plugin-a AppManager --plugin-b LifecycleManager
  %(prog)s wpeframework.log --verbose --output report.txt
        """
    )
    
    parser.add_argument('logfile', help='Path to wpeframework.log file')
    parser.add_argument('--plugin-a', help='Filter cross-plugin calls involving this plugin')
    parser.add_argument('--plugin-b', help='Filter cross-plugin calls between plugin-a and plugin-b')
    parser.add_argument('--verbose', '-v', action='store_true', help='Show detailed timing information')
    parser.add_argument('--output', '-o', help='Write report to file instead of stdout')
    
    args = parser.parse_args()
    
    # Analyze timing logs
    analyzer = TimingAnalyzer()
    
    print(f"Parsing log file: {args.logfile}")
    event_count = analyzer.parse_log_file(args.logfile)
    print(f"Parsed {event_count} timing events")
    
    print("Analyzing API timings...")
    analyzer.analyze_api_timings()
    
    print("Analyzing cross-plugin calls...")
    analyzer.analyze_cross_plugin_calls()
    
    print("Generating report...")
    report = analyzer.generate_report(
        plugin_a=args.plugin_a,
        plugin_b=args.plugin_b,
        verbose=args.verbose
    )
    
    # Output report
    if args.output:
        with open(args.output, 'w') as f:
            f.write(report)
        print(f"\nReport written to: {args.output}")
    else:
        print("\n" + report)


if __name__ == '__main__':
    main()
