local kinds = [
  'Test',
  'Controller',
  'Archive',
  'Directory',
  'Template',
  'Package',
  'Repository',
  'Symlink',
  'PackageManager',
];

local natives = [
  'getOS',
  'getDistro',
  'getArch',
  'getHostname',
  'getUsername',
];

{
  [name]: std.native(name)
  for name in natives
} +
{
  Labels(labels=null):
    (if labels != null then
       {
         metadata+: {
           labels+: labels,
         },
       }
     else
       {}),
  Annotations(annotations=null):
    (if annotations != null then
       {
         metadata+: {
           annotations+:
             (if std.isArray(annotations) then
                []
              else if std.isObject(annotations) then
                [
                  {
                    key: k,
                    value: annotations[k],
                  }
                  for k in std.objectFields(annotations)
                ]
              else
                []),
         },
       }
     else
       {}),
  Metadata(name, labels=null, annotations=null):
    {
      metadata+: {
        name: name,
      },
    } +
    $.Labels(labels) +
    $.Annotations(annotations),
  Manifest(kind, name, spec=null, labels=null, annotations=null):
    {
      kind: kind,
      spec: (if spec != null then spec else {}),
    } +
    $.Metadata(name, labels, annotations),
} +
{
  [kind + 'Manifest'](name, spec=null, labels=null, annotations=null):
    $.Manifest(kind, name, spec=spec, labels=labels, annotations=annotations)
  for kind in kinds
} +
{
  // ╭──────────╮
  // │ Archives │
  // ╰──────────╯
  ArchiveSpec(source, dest):
    {
      source: source,
      dest: dest,
    },
  Archive(name, source, dest, labels=null, annotations=null):
    $.ArchiveManifest(name, $.ArchiveSpec(source, dest), labels, annotations),
  Archives(data, labels=null, annotations=null):
    [
      $.Archive(name, data[name].source, data[name].dest, labels, annotations)
      for name in std.objectFields(data)
    ],

  // ╭─────────────╮
  // │ Directories │
  // ╰─────────────╯
  DirectorySpec(target):
    {
      target: target,
    },
  Directory(name, target, labels=null, annotations=null):
    $.DirectoryManifest(name, $.DirectorySpec(target), labels, annotations),
  Directories(names, labels=null, annotations=null):
    [
      $.Directory(name, labels, annotations)
      for name in names
    ],

  // ╭──────────╮
  // │ Packages │
  // ╰──────────╯
  PackageSpec(name, manager=null):
    {
      name: name,
    } +
    (if manager != null then
       {
         manager: manager,
       }
     else {}),
  Package(name, manager=null, labels=null, annotations=null):
    $.PackageManifest(name, $.PackageSpec(name, manager), labels, annotations),
  Packages(names, manager=null, labels=null, annotations=null):
    [
      $.Package(name, manager, labels=labels, annotations=annotations)
      for name in names
    ],

  // ╭─────────────────╮
  // │ Package Manager │
  // ╰─────────────────╯
  PackageManagerSpec(name):
    {
      type: name,
    },
  PackageManager(name, labels=null, annotations=null):
    $.PackageManagerManifest(name, $.PackageManagerSpec(name), labels=labels, annotations=annotations),
  PackageManagers(names, labels=null, annotations=null):
    [
      $.PackageManager(name, labels=labels, annotations=annotations)
      for name in names
    ],

  // ╭──────────────╮
  // │ Repositories │
  // ╰──────────────╯
  RepositorySpec(url, dest):
    {
      url: url,
      destination: dest,
    },
  Repository(name, url, dest, labels=null, annotations=null):
    $.RepositoryManifest(name, $.RepositorySpec(url, dest), labels=labels, annotations=annotations),
  Repositories(data, labels=null, annotations=null):
    [
      $.Repository(name, data[name].url, data[name].dest, labels=labels, annotations=annotations)
      for name in std.objectFields(data)
    ],

  // ╭──────────╮
  // │ Symlinks │
  // ╰──────────╯
  SymlinkSpec(source, target):
    {
      source: source,
      target: target,
    },
  Symlink(name, source, target, labels=null, annotations=null):
    $.SymlinkManifest(name, $.SymlinkSpec(source, target), labels=labels, annotations=annotations),
  Symlinks(data, labels=null, annotations=null):
    [
      $.Symlink(name, data[name].source, data[name].target, labels=labels, annotations=annotations)
      for name in std.objectFields(data)
    ],
}
