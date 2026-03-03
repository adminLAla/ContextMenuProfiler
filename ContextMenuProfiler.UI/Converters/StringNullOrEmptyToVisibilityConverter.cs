using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace ContextMenuProfiler.UI.Converters
{
    public class StringNullOrEmptyToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string? str = value as string;
            // If parameter is "Inverse", then:
            // Null/Empty -> Collapsed
            // Not Empty -> Visible
            
            bool isNullOrEmpty = string.IsNullOrEmpty(str);
            
            if (parameter is string paramStr && paramStr.Equals("Inverse", StringComparison.OrdinalIgnoreCase))
            {
                return isNullOrEmpty ? Visibility.Collapsed : Visibility.Visible;
            }

            // Default:
            // Null/Empty -> Visible (Show SymbolIcon when no custom icon)
            // Not Empty -> Collapsed (Hide SymbolIcon when custom icon exists)
            return isNullOrEmpty ? Visibility.Visible : Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
