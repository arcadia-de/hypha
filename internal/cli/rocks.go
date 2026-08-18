package cli

import (
	"fmt"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
)

func HandleInstallLuarocksPackage(cmd *cobra.Command, args []string) error {
	orc, err := hypha.NewOrchestratorWithDefaultConfig()
	if err != nil {
		return fmt.Errorf("failed to create new Orchestrator with default config: %v", err)
	}
	defer orc.Close()

	luarocks, err := hypha.GetPackageManager("luarocks")
	if err != nil {
		return err
	}

	for _, pkg := range args {
		fmt.Printf("installing %s\n", pkg)
		luarocks.Install(pkg)
	}

	return nil
}

func HandleUninstallLuarocksPackage(cmd *cobra.Command, args []string) error {
	return fmt.Errorf("not implemented")
}

func HandleListLuarocksPackages(cmd *cobra.Command, args []string) error {
	return fmt.Errorf("not implemented")
}

func CreateRocksInstallCommand() *cobra.Command {
	return &cobra.Command{
		Use:   "install ids",
		Short: "Install a luarocks package",
		Args:  cobra.MinimumNArgs(1),
		RunE:  HandleInstallLuarocksPackage,
	}
}

func CreateRocksUninstallCommand() *cobra.Command {
	return &cobra.Command{
		Use:   "uninstall",
		Short: "Uninstall a luarocks package",
		Args:  cobra.MinimumNArgs(1),
		RunE:  HandleUninstallLuarocksPackage,
	}
}

func CreateRocksListCommand() *cobra.Command {
	return &cobra.Command{
		Use:   "list",
		Short: "List installed luarocks packages",
		RunE:  HandleListLuarocksPackages,
	}
}

func init() {
	rocksCmd := &cobra.Command{
		Use:   "rocks [package...]",
		Short: "Manipulate luarocks packages",
	}
	rocksCmd.AddCommand(CreateRocksListCommand())
	rocksCmd.AddCommand(CreateRocksInstallCommand())
	rocksCmd.AddCommand(CreateRocksUninstallCommand())

	RootCmd.AddCommand(rocksCmd)
}
