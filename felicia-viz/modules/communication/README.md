# Felicia-viz communication

## How to add protobuf

```bash
npx pbjs -t json ../../../felicia/core/protobuf/*.proto ../../../felicia/drivers/**/*.proto > src/proto_bundle/felicia_proto_bundle.json
```