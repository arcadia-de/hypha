local hypha = import 'lib/hypha.libsonnet';

{
  kind: 'Test',
  metadata: {
    name: 'jsonnet-test',
  },
  spec: {},
} +
hypha.Annotations({
  distro: hypha.getDistro(),
})
