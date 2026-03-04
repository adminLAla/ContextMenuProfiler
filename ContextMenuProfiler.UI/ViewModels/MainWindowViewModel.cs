using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Collections.ObjectModel;
using Wpf.Ui.Controls;
using ContextMenuProfiler.UI.Views.Pages;
using ContextMenuProfiler.UI.Core.Services;
using System;
using System.Windows.Controls;
using System.Threading.Tasks;
using System.Windows.Threading;
using System.Reflection;

namespace ContextMenuProfiler.UI.ViewModels
{
    public partial class MainWindowViewModel : ObservableObject
    {
        [ObservableProperty]
        private string _applicationTitle = LocalizationService.Instance["App.Title"];

        [ObservableProperty]
        private HookStatus _currentHookStatus = HookStatus.Disconnected;

        [ObservableProperty]
        private string _hookStatusMessage = LocalizationService.Instance["Hook.NotInjected"];

        [ObservableProperty]
        private string _hookButtonText = LocalizationService.Instance["Hook.Inject"];

        [ObservableProperty]
        private string _statusBarVersionText = "";

        private readonly DispatcherTimer _statusTimer;
        private readonly string _appVersion;

        public MainWindowViewModel()
        {
            _appVersion = ResolveAppVersion();
            LocalizationService.Instance.PropertyChanged += (_, e) =>
            {
                if (e.PropertyName == "Item[]")
                {
                    ApplyLocalization();
                    _ = UpdateHookStatus();
                }
            };

            ApplyLocalization();
            _statusTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(2) };
            _statusTimer.Tick += async (s, e) => await UpdateHookStatus();
            _statusTimer.Start();
            _ = UpdateHookStatus();
        }

        private async Task UpdateHookStatus()
        {
            if (HookService.Instance.IsBusy) return;

            CurrentHookStatus = await HookService.Instance.GetStatusAsync();
            switch (CurrentHookStatus)
            {
                case HookStatus.Disconnected:
                    HookStatusMessage = LocalizationService.Instance["Hook.NotInjected"];
                    HookButtonText = LocalizationService.Instance["Hook.Inject"];
                    break;
                case HookStatus.Injected:
                    HookStatusMessage = LocalizationService.Instance["Hook.InjectedIdle"];
                    HookButtonText = LocalizationService.Instance["Hook.Eject"];
                    break;
                case HookStatus.Active:
                    HookStatusMessage = LocalizationService.Instance["Hook.Active"];
                    HookButtonText = LocalizationService.Instance["Hook.Eject"];
                    break;
            }
        }

        private void ApplyLocalization()
        {
            ApplicationTitle = LocalizationService.Instance["App.Title"];
            StatusBarVersionText = string.Format(LocalizationService.Instance["Status.VersionFormat"], _appVersion);
            MenuItems = new ObservableCollection<object>
            {
                new NavigationViewItem
                {
                    Content = LocalizationService.Instance["Nav.Dashboard"],
                    Icon = new SymbolIcon { Symbol = SymbolRegular.Home24 },
                    TargetPageType = typeof(DashboardPage)
                },
                new NavigationViewItem
                {
                    Content = LocalizationService.Instance["Nav.Settings"],
                    Icon = new SymbolIcon { Symbol = SymbolRegular.Settings24 },
                    TargetPageType = typeof(SettingsPage)
                }
            };

            TrayMenuItems = new ObservableCollection<System.Windows.Controls.MenuItem>
            {
                new System.Windows.Controls.MenuItem { Header = LocalizationService.Instance["Tray.Home"], Tag = "home" }
            };
        }

        private static string ResolveAppVersion()
        {
            var version = Assembly.GetExecutingAssembly().GetName().Version;
            if (version == null)
            {
                return "dev";
            }

            if (version.Build > 0)
            {
                return $"{version.Major}.{version.Minor}.{version.Build}";
            }

            return $"{version.Major}.{version.Minor}";
        }

        [RelayCommand]
        private async Task ToggleHook()
        {
            if (CurrentHookStatus == HookStatus.Disconnected)
            {
                await HookService.Instance.InjectAsync();
            }
            else
            {
                await HookService.Instance.EjectAsync();
            }
            await UpdateHookStatus();
        }

        [ObservableProperty]
        private ObservableCollection<object> _menuItems = new();

        [ObservableProperty]
        private ObservableCollection<object> _footerMenuItems = new()
        {
        };
        
        [ObservableProperty]
        private ObservableCollection<System.Windows.Controls.MenuItem> _trayMenuItems = new();
    }
}
