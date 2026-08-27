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

{
  Labels(labels):
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
  Manifest(kind, name, spec=null, labels=null, annotations=null):
    {
      kind: kind,
      metadata: {
        name: name,
      },
      spec: (if spec != null then spec else {}),
    } +
    $.Labels(labels) +
    $.Annotations(annotations),
} +
{
  [kind + 'Manifest'](name, spec=null, labels=null, annotations=null):
    $.Manifest(kind, name, spec=spec, labels=labels, annotations=annotations)
  for kind in kinds
}
