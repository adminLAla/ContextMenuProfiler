using ContextMenuProfiler.UI.ViewModels;
using System.Windows.Controls;

namespace ContextMenuProfiler.UI.Views.Pages
{
    public partial class SettingsPage : Page
    {
        public SettingsPage()
        {
            InitializeComponent();
            DataContext = new SettingsViewModel();
        }
    }
}
