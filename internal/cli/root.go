package cli

import (
	"fmt"
	"os"
	"strings"

	"github.com/arcadia-de/hypha/internal/hypha"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
)

var Verbose bool
var RootCmd = &cobra.Command{
	Use:   "hypha",
	Short: "A dotfile manager",
	Long:  "A dotfile manager",
	PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
		return initConfig(cmd)
	},
}

func Execute() error {
	if err := RootCmd.PersistentFlags().Parse(os.Args[1:]); err != nil {
		// ignore?
	}

	if flag := RootCmd.PersistentFlags().Lookup("config-dir"); flag != nil {
		if err := viper.BindPFlag("config-dir", flag); err != nil {
			return fmt.Errorf("failed to bind early config-dir flag: %w", err)
		}
	}

	config_dir, err := hypha.EnsureConfigDirExists()
	if err != nil {
		return err
	}
	hypha.InitHypha(config_dir)

	kinds := hypha.GetAllControllerKinds()
	RootCmd.AddCommand(CreateGenCommand(kinds))
	RootCmd.AddCommand(CreateDescribeCommand(kinds))
	RootCmd.AddCommand(CreateListCommand(kinds))
	return RootCmd.Execute()
}

func initConfig(cmd *cobra.Command) error {
	viper.SetEnvPrefix("HYPHA")
	viper.SetEnvKeyReplacer(strings.NewReplacer("-", "_", ".", "_"))
	viper.AutomaticEnv()
	return viper.BindPFlags(cmd.Flags())
}

func getDefaultConfigDir() (string, error) {
	xdg_config_dir, exists := os.LookupEnv("XDG_CONFIG_HOME")
	if exists {
		return fmt.Sprintf("%s/hypha", xdg_config_dir), nil
	}

	home, exists := os.LookupEnv("HOME")
	if exists {
		return fmt.Sprintf("%s/.config/hypha", home), nil
	}

	cwd, err := os.Getwd()
	if err != nil {
		return "", err
	}

	return cwd, nil
}

func getDefaultCacheDir() (string, error) {
	xdg_config_dir, exists := os.LookupEnv("XDG_CACHE_HOME")
	if exists {
		return fmt.Sprintf("%s/hypha", xdg_config_dir), nil
	}

	home, exists := os.LookupEnv("HOME")
	if exists {
		return fmt.Sprintf("%s/.cache/hypha", home), nil
	}

	return "", fmt.Errorf("failed to get default cache dir")
}

func getDefaultStateDir() (string, error) {
	xdg_config_dir, exists := os.LookupEnv("XDG_STATE_HOME")
	if exists {
		return fmt.Sprintf("%s/hypha", xdg_config_dir), nil
	}

	home, exists := os.LookupEnv("HOME")
	if exists {
		return fmt.Sprintf("%s/.local/state/hypha", home), nil
	}

	return "", fmt.Errorf("failed to get default state dir")
}

func init() {
	RootCmd.AddGroup(&cobra.Group{
		ID:    "config",
		Title: "Configuration Commands",
	})

	RootCmd.AddGroup(&cobra.Group{
		ID:    "inspection",
		Title: "Inspection Commands",
	})

	RootCmd.AddGroup(&cobra.Group{
		ID:    "development",
		Title: "Development Commands",
	})

	RootCmd.AddGroup(&cobra.Group{
		ID:    "resources",
		Title: "Resource Commands",
	})

	config_dir, err := getDefaultConfigDir()
	if err != nil {
		fmt.Printf("failed to get default config dir: %v", err)
		os.Exit(1)
	}
	RootCmd.PersistentFlags().StringP("config-dir", "", config_dir, "The configuration dir for hypha")

	cache_dir, err := getDefaultCacheDir()
	if err != nil {
		fmt.Printf("failed to get default cache dir: %v", err)
		os.Exit(1)
	}
	RootCmd.PersistentFlags().StringP("cache-dir", "", cache_dir, "The cache dir for hypha")

	state_dir, err := getDefaultStateDir()
	if err != nil {
		fmt.Printf("failed to get default state dir: %v", err)
		os.Exit(1)
	}
	RootCmd.PersistentFlags().StringP("state-dir", "", state_dir, "The state dir for hypha")

	RootCmd.PersistentFlags().BoolVarP(&Verbose, "verbose", "v", false, "add more detailed output")
}
