package hypha

import (
	"fmt"
	"github.com/Masterminds/semver/v3"
)

const CurrentManifestVersion = "v1.0.0"

func ValidateManifestVersion(version string) error {
	c, err := semver.NewConstraint(">= " + CurrentManifestVersion)
	if err != nil {
		return err
	}

	v, err := semver.NewVersion(version)
	if err != nil {
		return err
	}

	valid := c.Check(v)
	if !valid {
		return fmt.Errorf("version `%s` does not meet the constraint of `>= %s`", version, CurrentManifestVersion)
	}

	return nil
}
