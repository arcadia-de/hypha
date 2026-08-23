{
  Manifest(kind, name, spec=null, labels=null, annotations=null):
    {
      kind: kind,
      metadata: {
        name: name,
      },
    } +
    (if spec != null then
       {
         spec: spec,
       }
     else
       {}) +
    (if labels != null then
       {
         metadata+: {
           labels+: labels,
         },
       }
     else
       {}) +
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
}
