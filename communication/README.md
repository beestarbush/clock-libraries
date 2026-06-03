# communication

Shared C++/Qt communication library for app/backend protocol code.

## Goals

- Centralize JSON-RPC wire contract (methods/topics/frames).
- Use explicit shared-library ownership naming (`Common::Communication::<Domain>`).
- Let backend and app consume the same protocol implementation.

## Modules

- `websocket/Types.h`
  - Shared enums and wire string conversion helpers.
- `websocket/Frame.*`
  - JSON-RPC frame builders and parsers.
- `websocket/client/Service.*`
  - Reusable Qt WebSocket client service with request correlation.
- `websocket/server/Service.*`
  - Reusable Qt WebSocket server service with request and publish handling.
- `websocket/SubscriptionHelper.*`
  - Shared startup/bootstrap + topic subscription helper.
- `configuration/DeviceConfiguration.*`
  - Shared configuration schema mapping and serialization helpers.
- `protocol/MethodCatalog.*`
  - Canonical method/topic lists for completeness checks.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Notes

- This library is intentionally communication-only.
- Hardware access and backend runtime behavior stay outside this module.
- Next migration step is backend-first adoption, then app migration.
