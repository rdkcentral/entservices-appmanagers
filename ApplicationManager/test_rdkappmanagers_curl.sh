#!/usr/bin/env bash
set -euo pipefail

JSONRPC_URL="${JSONRPC_URL:-http://localhost:9998/jsonrpc}"
PLUGIN_METHOD="org.rdk.RDKAppManagers.1.request"
APP_ID="${1:-xumo}"
FALLBACK_APP_ID="${FALLBACK_APP_ID:-YouTube}"
STATEFUL_DELAY_SEC="${STATEFUL_DELAY_SEC:-3}"

SYSTEM_SETTING_KEY="${SYSTEM_SETTING_KEY:-system.acceptcasting}"
SYSTEM_SETTING_VALUE="${SYSTEM_SETTING_VALUE:-true}"
TEST_PREF_KEY="${TEST_PREF_KEY:-forceallappslaunchable}"
TEST_PREF_VALUE="${TEST_PREF_VALUE:-true}"
TEST_PREF_PIN="${TEST_PREF_PIN:-0}"

call() {
  local title="$1"
  local payload="$2"
  echo
  echo "===== ${title} ====="
  curl -sS -H "Content-Type: application/json" -X POST -d "$payload" "$JSONRPC_URL"
  echo
}

json_escape() {
  local input="$1"
  input=${input//\\/\\\\}
  input=${input//\"/\\\"}
  input=${input//$'\n'/\\n}
  input=${input//$'\r'/\\r}
  input=${input//$'\t'/\\t}
  printf '%s' "$input"
}

# URL request wrapper using your required format
call_request() {
  local id="$1"
  local flags="$2"
  local url="$3"
  local queryParams="$4"
  local body="$5"

  local escapedQueryParams
  local escapedBody
  escapedQueryParams=$(json_escape "$queryParams")
  escapedBody=$(json_escape "$body")

  local payload
  payload=$(cat <<EOF
{"jsonrpc":"2.0","id":${id},"method":"${PLUGIN_METHOD}","params":{"flags":${flags},"url":"${url}","headers":"","queryParams":"${escapedQueryParams}","body":"${escapedBody}"}}
EOF
)

  call "${url}" "$payload"
}

# ------------------------------
# Request() endpoint tests
# ------------------------------
call_request 1 1 "/as/apps" "{}" ""

QP_LAUNCH='{"appId":"'"${APP_ID}"'"}'
LAUNCH_BODY='{"mode":"","intent":"{\\"action\\":\\"launch\\",\\"context\\":{\\"source\\":\\"voice\\"}}","launchArgs":""}'
call_request 2 2 "/as/apps/action/launch" "$QP_LAUNCH" "$LAUNCH_BODY"

QP_APP='{"appId":"'"${APP_ID}"'"}'
call_request 3 2 "/as/apps/action/close" "$QP_APP" ""
call_request 4 2 "/as/apps/action/kill" "$QP_APP" ""
call_request 5 2 "/as/apps/action/focus" "$QP_APP" ""
call_request 6 2 "/as/apps/action/reset" "$QP_APP" ""
call_request 7 2 "/as/apps/action/reset" "{}" ""
call_request 8 1 "/as/system/stats" "{}" ""

# ------------------------------
# Stateful operation checks
# launch -> wait -> focus/close/kill (primary app)
# then same for fallback app id
# ------------------------------
echo
echo "===== stateful-sequence(primary:${APP_ID}) ====="
call_request 9 2 "/as/apps/action/launch" "$QP_LAUNCH" "$LAUNCH_BODY"
echo "sleep ${STATEFUL_DELAY_SEC}s"
sleep "${STATEFUL_DELAY_SEC}"
call_request 10 2 "/as/apps/action/focus" "$QP_APP" ""
call_request 11 2 "/as/apps/action/close" "$QP_APP" ""
call_request 12 2 "/as/apps/action/kill" "$QP_APP" ""

QP_FALLBACK='{"appId":"'"${FALLBACK_APP_ID}"'"}'
LAUNCH_BODY_FALLBACK='{"mode":"","intent":"{\\"action\\":\\"launch\\",\\"context\\":{\\"source\\":\\"voice\\"}}","launchArgs":""}'

echo
echo "===== stateful-sequence(fallback:${FALLBACK_APP_ID}) ====="
call_request 13 2 "/as/apps/action/launch" "$QP_FALLBACK" "$LAUNCH_BODY_FALLBACK"
echo "sleep ${STATEFUL_DELAY_SEC}s"
sleep "${STATEFUL_DELAY_SEC}"
call_request 14 2 "/as/apps/action/focus" "$QP_FALLBACK" ""
call_request 15 2 "/as/apps/action/close" "$QP_FALLBACK" ""
call_request 16 2 "/as/apps/action/kill" "$QP_FALLBACK" ""

# ------------------------------
# TestPreferences / SystemSettings interface tests
# ------------------------------

# getSystemSetting(name="") -> fetch all system settings
call "getSystemSetting(all)" \
'{"jsonrpc":"2.0","id":200,"method":"org.rdk.RDKAppManagers.1.getSystemSetting","params":{"name":""}}'

# getSystemSetting(name)
call "getSystemSetting(${SYSTEM_SETTING_KEY})" \
'{"jsonrpc":"2.0","id":201,"method":"org.rdk.RDKAppManagers.1.getSystemSetting","params":{"name":"'"${SYSTEM_SETTING_KEY}"'"}}'

# setSystemSetting(name, value)
call "setSystemSetting(${SYSTEM_SETTING_KEY}=${SYSTEM_SETTING_VALUE})" \
'{"jsonrpc":"2.0","id":202,"method":"org.rdk.RDKAppManagers.1.setSystemSetting","params":{"name":"'"${SYSTEM_SETTING_KEY}"'","value":"'"${SYSTEM_SETTING_VALUE}"'"}}'

# getTestPreference(name="") -> fetch all test preferences
call "getTestPreference(all)" \
'{"jsonrpc":"2.0","id":203,"method":"org.rdk.RDKAppManagers.1.getTestPreference","params":{"name":""}}'

# getTestPreference(name)
call "getTestPreference(${TEST_PREF_KEY})" \
'{"jsonrpc":"2.0","id":204,"method":"org.rdk.RDKAppManagers.1.getTestPreference","params":{"name":"'"${TEST_PREF_KEY}"'"}}'

# setTestPreference(name, value, pin)
call "setTestPreference(${TEST_PREF_KEY}=${TEST_PREF_VALUE}, pin=${TEST_PREF_PIN})" \
'{"jsonrpc":"2.0","id":205,"method":"org.rdk.RDKAppManagers.1.setTestPreference","params":{"name":"'"${TEST_PREF_KEY}"'","value":"'"${TEST_PREF_VALUE}"'","pin":'"${TEST_PREF_PIN}"'}}'

echo

echo "All test calls completed."
