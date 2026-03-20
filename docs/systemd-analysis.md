# Thunder Plugin Startup Analysis

> **Source logs:** `wpeframework.log` (35 480 lines, 82 SysLog plugin-lifecycle events) · `ntp.log`
> **Date of capture:** 2025-12-03 / 2026-03-20 (clock jump — see below)

---

## Clock Context (ntp.log)

The device booted with a **wrong system clock** (2025-12-03, drifted close to epoch).
`systemd-timesyncd` restored a saved timestamp to **2026-03-20** at ~boot+17.3 s.
NTP then fully synchronised at **boot+30 s** with a +73.7 s offset jump.
This creates **two timestamp phases** in `wpeframework.log`.

| Reference | Value |
|---|---|
| WPEFramework daemon start (old clock) | `2025-12-03T08:38:59.007Z` |
| WPEFramework start (real time) | `2026-03-20T12:05:45.576Z`  (boot+13 s) |
| Clock restored old → real | ~boot+17.3 s |
| NTP fully synchronised | `2026-03-20T12:06:02.570Z`  (boot+30 s, offset +73.7 s) |

---

## Phase 1 — Early Boot (boot+13 s → boot+20 s, old clock 2025-12-03)

`PluginInitializerService` activates and immediately dispatches a **large parallel burst of 8 plugins**, then drains the remaining queue mostly sequentially.

| WPE+sec | Action | Plugin | Tier |
|---|---|---|---|
| +0.86 s | Activated | Controller | — |
| +0.90 s | Activated | PluginInitializerService | — |
| +1.72 s | Activated | USBDevice | — |
| +2.23 s | Activated | Monitor | — |
| +2.61 s | Activated | LinchPinClientManager | — |
| +2.70 s | Activated | AppNotifications | — |
| +2.71 s | Activated | CryptographyExtAccess | — |
| +3.23 s | Activated | TextToSpeech | — |
| +3.70 s | Activated | MediaSettings | — |
| **+4.23 s** | **Activated** | **RDKWindowManager** | **T1** |
| **+4.32 s** | **Activated** | **TelemetryMetrics** | **T1** |
| +4.32 s | Activated | FirmwareUpdate | — |
| +5.02 s | Activated | SystemAudioPlayer | — |
| +5.35 s | Activated | PowerManager | — |
| +5.37 s | Activated | DeviceDiagnostics | — |
| +5.37 s | Activated | SystemMode | — |
| **+5.46 s** | **Activated** | **PersistentStore** | **T1** |
| +5.49 s | Activated | BackupManager | — |
| +5.69 s | Activated | USBMassStorage | — |
| **+5.82 s** | **Activated** | **AppStorageManager** | **T2** |
| +6.31 s | Activated | CloudStore | — |

### Phase 1 Parallel Bursts (ActivateJob dispatch)

| Burst | @WPE+ | Plugins dispatched |
|---|---|---|
| **1** | **+1.2 s** | Monitor, UsbDevice, TextToSpeech, **Cryptography**, **RDKWindowManager [T1]**, AppNotifications, LinchPinClient, MediaSettings |
| 2 | +1.7 s | FirmwareUpdate |
| 3 | +2.2 s | CloudStore |
| 4 | +2.6 s | **TelemetryMetrics [T1]** |
| 5 | +2.7 s | PowerManager |
| 6 | +2.9 s | SystemAudioPlayer |
| 7 | +3.2 s | BackupManager |
| 8 | +3.7 s | SystemMode |
| 9 | +4.2 s | **PersistentStore [T1]** |
| 10 | +4.3 s | DeviceDiagnostics, NetworkManager |
| 11 | +5.0 s | UsbMassStorage |
| 12 | +5.4 s | AppGateway, **AppStorageManager [T2]** |
| 13 | +5.9 s | System, Account |
| 14 | +6.0 s | Xcast, UserSettings |
| 15 | +6.3 s | **PackageManagerRDKEMS [T3]** |

---

## Phase 2 — Post-NTP-Restore (boot+19 s → boot+115 s, real clock 2026-03-20)

After the clock is corrected, `PluginInitializerService` continues with the larger plugin set. Activations are mostly sequential with several smaller parallel bursts.

| boot+sec | Action | Plugin | Tier |
|---|---|---|---|
| +19.5 s | Activated | SystemServices | — |
| +20.3 s | Activated | AppGateway | — |
| +20.3 s | Activated | UserSettings | — |
| +20.3 s | Activated | Account | — |
| **+20.3 s** | **Activated** | **PackageManager** | **T3** |
| +20.5 s | Activated | XCast | — |
| +20.5 s | Activated | NetworkManager | — |
| +21.2 s | Activated | SharedStorage | — |
| +21.3 s | Activated | AVInput | — |
| +21.3 s | Activated | FrontPanel | — |
| +21.4 s | Activated | DisplaySettings | — |
| +21.7 s | Activated | DeviceInfo | — |
| +21.7 s | Activated | HdmiCecSource | — |
| **+21.7 s** | **Activated** | **OCIContainer** | **T1** |
| +22.2 s | Activated | DisplayInfo | — |
| +22.3 s | Activated | LaunchDelegate | — |
| +22.3 s | Activated | FbAsLinchPin | — |
| **+22.6 s** | **Activated** | **PreinstallManager** | **T4** |
| +22.6 s | Activated | PlayerInfo | — |
| +23.1 s | Activated | WiFiManager | — |
| +23.1 s | Activated | Network | — |
| +23.1 s | Activated | Telemetry | — |
| +23.3 s | Activated | RemoteControl | — |
| +23.3 s | Activated | VoiceControl | — |
| **+23.5 s** | **Activated** | **RuntimeManager** | **T5** |
| +23.9 s | Activated | MaintenanceManager | — |
| +23.9 s | Activated | AuthService | — |
| **+24.1 s** | **Activated** | **LifecycleManager** | **T6** |
| +24.6 s | Activated | WifiMetrics | — |
| +25.5 s | Activated | FbEntos | — |
| +25.8 s | Activated | AppGatewayCommon | — |
| +26.1 s | Activated | Privacy | — |
| **+26.1 s** | **Activated** | **AppManager** | **T7** |
| +26.3 s | Activated | Bluetooth | — |
| +27.0 s | Activated | MediaSystem | — |
| +27.0 s | Activated | XvpClient | — |
| +27.0 s | Activated | FbPrivacy | — |
| +27.1 s | Activated | SecManager | — |
| **+27.5 s** | **Activated** | **DownloadManager** | **T8** |
| +27.7 s | Activated | MediaServices | — |
| +27.8 s | Activated | Analytics | — |
| +27.8 s | Activated | FbAdvertising | — |
| +27.9 s | Activated | FbDiscovery | — |
| +29.3 s | Activated | ContentProtection | — |
| +29.3 s | Activated | FbMetrics | — |
| +30.0 s | Activated | DeviceProvisioning | — |
| **+104.8 s** | **Activated** | **OCDM** | — ⚠️ |
| **+114.7 s** | **Activated** | **IPControl** | — ⚠️ |

### Phase 2 Parallel Bursts (ActivateJob dispatch)

| Burst | @boot+ | Plugins dispatched |
|---|---|---|
| 1 | +19.7 s | SharedStorage |
| 2 | +20.2 s | HdmiCecSource |
| **3** | **+20.3 s** | AVInput, DisplaySettings, HdcpProfile, FrontPanel |
| 4 | +20.5 s | DeviceInfo |
| 5 | +20.6 s | **OCIContainer [T1]** |
| 6 | +21.3 s | PlayerInfo |
| 7 | +21.4 s | DisplayInfo |
| 8 | +21.5 s | Telemetry |
| 9 | +21.6 s | LaunchDelegate, FbAsLinchPin |
| **10** | **+21.7 s** | **PreinstallManager [T4]**, Wifi, Network |
| 11 | +22.3 s | WifiMetrics |
| 12 | +23.0 s | RemoteControl |
| 13 | +23.2 s | VoiceControl |
| 14 | +23.2 s | **RuntimeManager [T5]** |
| 15 | +23.5 s | AuthService |
| 16 | +23.6 s | MaintenanceManager |
| 17 | +24.0 s | **LifecycleManager [T6]** |
| **18** | **+24.5 s** | FbEntos, DeviceProvisioning, Privacy, AppGatewayCommon, SecManager, **AppManager [T7]** |
| 19 | +25.5 s | Bluetooth |
| **20** | **+26.6 s** | XvpClient, FbPrivacy, Analytics |
| 21 | +26.8 s | **DownloadManager [T8]** |
| 22 | +27.6 s | FbDiscovery, FbAdvertising |
| 23 | +28.1 s | ContentProtection |
| 24 | +28.9 s | FbMetrics |
| 25 | +104.2 s | OCDM |
| 26 | +112.4 s | IPControl |

---

## Tier Chain — Time from Real Boot Epoch

```
Tier 0  wpeframework       boot+13.0s   (reference)
Tier 1  RDKWindowManager   boot+17.2s   (+4.2s from T0)
Tier 1  TelemetryMetrics   boot+17.3s   (parallel with RDKWindowManager)
Tier 1  PersistentStore    boot+18.5s   (+5.5s from T0)
Tier 1  OCIContainer       boot+21.7s   (+8.7s from T0)   ← lags T1 peers
Tier 2  AppStorageManager  boot+18.8s   (+1.5s after first T1 ready)
Tier 3  PackageManager     boot+20.3s   (+1.5s from T2)
Tier 4  PreinstallManager  boot+22.6s   (+2.3s from T3)
Tier 5  RuntimeManager     boot+23.5s   (+0.9s from T4)
Tier 6  LifecycleManager   boot+24.1s   (+0.6s from T5)
Tier 7  AppManager         boot+26.1s   (+2.0s from T6)
Tier 8  DownloadManager    boot+27.5s   (+1.4s from T7)
```

**Total chain: WPEFramework start → DownloadManager ready = ~14.5 s of real elapsed time**

---

## Parallelism Statistics

| Metric | Phase 1 (pre-NTP, boot+0–7 s) | Phase 2 (post-NTP, boot+19 s+) |
|---|---|---|
| Total plugins dispatched | 26 | 40 |
| Parallel bursts (≥2 together) | **5** | **6** |
| Plugins in those parallel bursts | 16 (62%) | 20 (50%) |
| Solo/sequential dispatches | 10 | 20 |

### Largest parallel groups

| Phase | @boot+ | Count | Plugins |
|---|---|---|---|
| Phase 1 | +1.2 s | **8** | Monitor, UsbDevice, TextToSpeech, Cryptography, **RDKWindowManager**, AppNotifications, LinchPinClient, MediaSettings |
| Phase 2 | +20.3 s | 4 | AVInput, DisplaySettings, HdcpProfile, FrontPanel |
| Phase 2 | +21.7 s | 3 | **PreinstallManager [T4]**, Wifi, Network |
| Phase 2 | +24.5 s | **6** | FbEntos, DeviceProvisioning, Privacy, AppGatewayCommon, SecManager, **AppManager [T7]** |
| Phase 2 | +26.6 s | 3 | XvpClient, FbPrivacy, Analytics |

---

## Key Observations

1. **OCIContainer is the slowest Tier 1 plugin.**
   Activates at boot+21.7 s vs RDKWindowManager at boot+17.2 s — a **4.5 s lag** within the same tier. It is dispatched in a separate burst, not alongside the initial T1 wave.

2. **PersistentStore and AppStorageManager (T1/T2) activate unusually early.**
   Both complete in Phase 1 (boot+18.5 s / +18.8 s), before most Phase 2 plugins are even dispatched. This suggests they have no hard dependency on later services.

3. **OCDM is anomalously late.**
   Activates at boot+104.8 s — **~77 s after all other plugins** are ready. IPControl follows 10 s later at +114.7 s. Any DRM-dependent content pipeline (playback, protected downloads) is blocked until then.

4. **PluginInitializerService drives all activation.**
   It dispatches plugins in waves. The first wave is a single burst of 8 concurrent activations; subsequent waves are largely one-at-a-time, creating the stairstep pattern from Tier 3 onwards.

5. **The Tier 0→8 sequential chain is tight (14.5 s total).**
   Inter-tier gaps are small (0.6–4.5 s), meaning each tier plugin completes quickly. The bottleneck is **Tier 1's OCIContainer**, which activates 3 s after AppStorageManager has already become ready.

6. **~55% of all dispatched plugins are launched in parallel bursts.**
   The system makes good use of parallelism, especially in Phase 1 (62% of plugins in bursts).
