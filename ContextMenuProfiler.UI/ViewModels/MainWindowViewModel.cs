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

namespace ContextMenuProfiler.UI.ViewModels
{
    public partial class MainWindowViewModel : ObservableObject
    {
        [ObservableProperty]
        private string _applicationTitle = "Context Menu Profiler";

        [ObservableProperty]
        private HookStatus _currentHookStatus = HookStatus.Disconnected;

        [ObservableProperty]
        private string _hookStatusMessage = "Hook: Disconnected";

        [ObservableProperty]
        private string _hookButtonText = "Inject Hook";

        private readonly DispatcherTimer _statusTimer;

        public MainWindowViewModel()
        {
            _statusTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(2) };
            _statusTimer.Tick += async (s, e) => await UpdateHookStatus();
            _statusTimer.Start();
            _ = UpdateHookStatus();
        }

        private async Task UpdateHookStatus()
        {
            CurrentHookStatus = await HookService.Instance.GetStatusAsync();
            switch (CurrentHookStatus)
            {
                case HookStatus.Disconnected:
                    HookStatusMessage = "Hook: Not Injected";
                    HookButtonText = "Inject Hook";
                    break;
                case HookStatus.Injected:
                    HookStatusMessage = "Hook: Injected (Idle)";
                    HookButtonText = "Eject Hook";
                    break;
                case HookStatus.Active:
                    HookStatusMessage = "Hook: Active";
                    HookButtonText = "Eject Hook";
                    break;
            }
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
        private ObservableCollection<object> _menuItems = new()
        {
            new NavigationViewItem
            {
                Content = "Dashboard",
                Icon = new SymbolIcon { Symbol = SymbolRegular.Home24 },
                TargetPageType = typeof(DashboardPage)
            },
            new NavigationViewItem
            {
                Content = "Settings",
                Icon = new SymbolIcon { Symbol = SymbolRegular.Settings24 },
                TargetPageType = typeof(SettingsPage)
            }
        };

        [ObservableProperty]
        private ObservableCollection<object> _footerMenuItems = new()
        {
        };
        
        [ObservableProperty]
        private ObservableCollection<System.Windows.Controls.MenuItem> _trayMenuItems = new()
        {
             new System.Windows.Controls.MenuItem { Header = "Home", Tag = "home" }
        };
    }
}
