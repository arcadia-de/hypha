
local shared = import "shared_config";
[
	// TODO: Declare your manifests here
	shared.Package("git") + 
		shared.Labels([
			"test",
			"example",
		]),
]