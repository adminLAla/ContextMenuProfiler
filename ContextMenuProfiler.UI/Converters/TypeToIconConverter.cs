using System;
using System.Globalization;
using System.Windows.Data;
using Wpf.Ui.Controls;

namespace ContextMenuProfiler.UI.Converters
{
    public class TypeToIconConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is string type)
            {
                if (type == "UWP") return SymbolRegular.AppGeneric24;
                if (type == "COM") return SymbolRegular.PuzzlePiece24; // Default generic icon
                if (type == "Static") return SymbolRegular.WindowConsole20;
            }
            return SymbolRegular.PuzzlePiece24; // Default fallback
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}