using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Diagnostics;
using System.Windows;
using System;

namespace ContextMenuProfiler.UI.ViewModels
{
    public partial class SettingsViewModel : ObservableObject
    {
        [RelayCommand]
        private void RestartExplorer()
        {
            if (MessageBox.Show("Are you sure you want to restart Windows Explorer?\nThis will temporarily close all folder windows and the taskbar.", "Confirm Restart", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
            {
                try
                {
                    foreach (var process in Process.GetProcessesByName("explorer"))
                    {
                        process.Kill();
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Failed to restart Explorer: {ex.Message}", "Error");
                }
            }
        }
    }
}
