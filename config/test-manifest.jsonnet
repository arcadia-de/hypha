local shared = import 'shared_config';

local Manifest(id, kind, spec=null) =
  {
    id: id,
    kind: kind,
  } +
  (if spec != null then { spec: spec } else {});

local Annotations(data) =
  {
    metadata: {
      annotations: [
        {
          key: k,
          value: data[k],
        }
        for k in std.objectFields(data)
      ],
    },
  };

local Labels(values) =
  {
    metadata: {
      labels: (if std.isArray(values) then values else [values]),
    },
  };

local DependsOn(values) =
  {
    depends_on: (if std.isArray(values) then values else [values]),
  };

local TestManifest(id) = Manifest(id, 'Test');
local ControllerManifest(id) = Manifest(id, 'Controller');
local ArchiveManifest(id) = Manifest(id, 'Archive');
local DirectoryManifest(id) = Manifest(id, 'Directory');
local PackageManifest(id) = Manifest(id, 'Package');
local RepositoryManifest(id) = Manifest(id, 'Repository');

local SymlinkManifest(id, source, target) =
  Manifest(id, 'Symlink', {
    source: source,
    target: target,
  });

local TemplateManifest(id) = Manifest(id, 'Template');

[
  TestManifest('test-0') +
  Annotations({
    os: shared.getOperatingSystemName(),
  }) +
  Labels([
    'debug',
  ]),

  SymlinkManifest('symlink-0', '/home/tazz/Projects/hypha/config/test-manifest.jsonnet', '/home/tazz/Projects/hypha/config/test.jsonnet'),
]
