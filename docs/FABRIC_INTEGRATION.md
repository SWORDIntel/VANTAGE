# FABRIC Integration (Optional)

VANTAGE can integrate with a larger multi-service ecosystem (“the Fabric”). This integration is **optional** and **disabled by default**.

## Design contract (what VANTAGE does)

- **Outbound-only telemetry** (one-way): VANTAGE only *emits* events. It does **not** read commands/requests from the Fabric.
- **Non-blocking**: telemetry sends use short timeouts and run best-effort; failures are dropped and do not slow shell startup.
- **Configurable docs root**: a configurable documentation root can be opened by aliases (`fabric-open-doc`).
- **Pipeline lifecycle naming**: lightweight wrappers for a standard lifecycle naming scheme are available when Fabric is enabled.

## Enabling Fabric integration

1. Copy `config/config.yaml.dist` → `config.yaml` (repo root) and edit:

```yaml
fabric:
  enabled: true
  docs_root: "/path/to/docs"
  telemetry:
    enabled: true
    transport: "unix"
    unix_socket: "/tmp/fabric-events.sock"
    timeout_ms: 50
    drop_on_fail: true
```

2. Enable/load the `fabric_integration` module (via your normal module enable flow).

## Commands

- `fabric-status`: prints whether Fabric integration is enabled and basic environment hints (tty/gui/headless) plus telemetry transport.
- `fabric-env`: prints the effective `FABRIC_*` configuration values.
- `fabric-open-doc <alias>`: opens a document by alias using `fabric.docs_root`.
  - Built-in aliases: `hardware`, `devices`, `roadmap`, `security`, `fabric`, `kitty`

## Telemetry

### What is emitted

Events are emitted as **one JSON line** per event, including:
- timestamp (`ts`)
- event type (`type`)
- optional identity tags (`node_id`, `device_id`, `layer_id`)
- event payload (`payload`) (caller-provided JSON object)

Current low-volume hook points included:
- **module load**: `type=module_load` for `fabric_integration`
- **installer completion**: `type=installer_complete` with `status=success|fail`

### What is never emitted by default

VANTAGE’s telemetry is designed to avoid secrets by default. In particular, the built-in hooks do **not** include:
- API tokens, credentials, private keys
- full command histories
- full environment dumps

If you add your own payloads, keep them minimal and non-sensitive.

### Transports

#### Unix socket (preferred)

Set:
- `fabric.telemetry.transport: "unix"`
- `fabric.telemetry.unix_socket: "/tmp/fabric-events.sock"`

VANTAGE will attempt to send via `nc -U` (or `socat` if available) with a short timeout.

#### HTTP

Set:
- `fabric.telemetry.transport: "http"`
- `fabric.telemetry.http_endpoint: "http://127.0.0.1:PORT/events"`

VANTAGE will POST the JSON line using `curl` with short connect/max timeouts.

## Troubleshooting

- **Nothing is emitted**: confirm `fabric.enabled=true` and `fabric.telemetry.enabled=true`, then run `fabric-env` and `fabric-status`.
- **Unix socket transport not working**: ensure the socket path exists and is writable, and that `nc -U` or `socat` is installed.
- **HTTP transport not working**: ensure `curl` exists and the endpoint is reachable locally.

## Security notes

- Telemetry is **outbound-only by default** and intentionally does **not** accept inbound control.
- Sends are best-effort with short timeouts, designed to avoid startup regressions.
- If you need bidirectional control, it must be implemented as a **separately enabled experimental mode** (not provided by default).

