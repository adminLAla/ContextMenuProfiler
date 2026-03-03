using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using ContextMenuProfiler.UI.Core.Services;

namespace ContextMenuProfiler.UI.Converters
{
    public class StatusToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string? param = parameter as string;

            if (param == "NotActive" && value is HookStatus hookStatus)
            {
                return hookStatus != HookStatus.Active ? Visibility.Visible : Visibility.Collapsed;
            }

            if (param == "NotUWP")
            {
                string? type = value as string;
                return (type != "UWP" && type != "UWP / Packaged COM") ? Visibility.Visible : Visibility.Collapsed;
            }

            if (param == "Fallback")
            {
                string? statusStr = value as string;
                bool isFallback = statusStr != null && (statusStr.Contains("Fallback") || statusStr.Contains("Error") || statusStr.Contains("Orphaned") || statusStr.Contains("Missing"));
                return isFallback ? Visibility.Visible : Visibility.Collapsed;
            }

            if (value is string status)
            {
                bool isWarning = status.StartsWith("Load Error") || 
                                 status.Contains("Exception") || 
                                 status.Contains("Failed") || 
                                 status.Contains("Not Registered") || 
                                 status.Contains("Invalid") || 
                                 status.Contains("Not Found") ||
                                 status.Contains("Fallback") ||
                                 status.Contains("No Menu") ||
                                 status.Contains("Orphaned") ||
                                 status.Contains("Missing");
                
                if (param == "Inverse")
                {
                    return isWarning ? Visibility.Collapsed : Visibility.Visible;
                }
                
                return isWarning ? Visibility.Visible : Visibility.Collapsed;
            }
            return Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}