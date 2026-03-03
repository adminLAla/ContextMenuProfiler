using System;
using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;

namespace ContextMenuProfiler.UI.Converters
{
    public class LoadTimeToBrushConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (parameter as string == "AdaptiveMode")
            {
                if (value is double width)
                {
                    return width < 900; // Threshold for hiding text labels
                }
                return false;
            }

            if (value is long ms)
            {
                // IsBackground? (parameter == "Background")
                bool isBackground = parameter as string == "Background";

                if (ms < 100)
                {
                    // Fast: Green/Success
                    // Background: Transparent or very light green
                    return isBackground ? Brushes.Transparent : new SolidColorBrush(Color.FromRgb(16, 124, 16)); // Green
                }
                else if (ms < 500)
                {
                    // Medium: Orange/Warning
                    // Background: Light Orange
                    return isBackground ? new SolidColorBrush(Color.FromArgb(40, 255, 140, 0)) : new SolidColorBrush(Color.FromRgb(180, 80, 0)); // Darker Orange for Text
                }
                else
                {
                    // Slow: Red/Critical
                    // Background: Light Red
                    return isBackground ? new SolidColorBrush(Color.FromArgb(40, 232, 17, 35)) : new SolidColorBrush(Color.FromRgb(200, 10, 20)); // Dark Red for Text
                }
            }
            return Brushes.Transparent;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}