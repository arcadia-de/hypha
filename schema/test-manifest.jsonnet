{
  'test-dir.json': {
    kind: 'Directory',
    metadata: {
      name: 'Test Directory',
      labels: [
        'test',
      ],
    },
    spec: {
      dir: '',
    },
  },
  'test-symlink.json': {
    kind: 'Symlink',
    metadata: {
      name: 'Test Symlink',
      labels: [
        'test',
      ],
    },
    spec: {
      source: '',
      target: '',
    },
  },
}
