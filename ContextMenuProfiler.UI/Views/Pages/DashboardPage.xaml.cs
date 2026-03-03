using ContextMenuProfiler.UI.ViewModels;
using System.Windows;
using System.Windows.Controls;

namespace ContextMenuProfiler.UI.Views.Pages
{
    public partial class DashboardPage : Page
    {
        public DashboardViewModel ViewModel { get; }

        public DashboardPage()
        {
            InitializeComponent();
            ViewModel = new DashboardViewModel();
            DataContext = ViewModel;
        }

        private void Page_Drop(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);
                if (files != null && files.Length > 0)
                {
                    string targetFile = files[0];
                    // Execute command on ViewModel
                    if (ViewModel.ScanFileCommand.CanExecute(targetFile))
                    {
                        ViewModel.ScanFileCommand.Execute(targetFile);
                    }
                }
            }
        }

        private void Page_DragOver(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                e.Effects = DragDropEffects.Copy;
                e.Handled = true;
            }
            else
            {
                e.Effects = DragDropEffects.None;
            }
        }
    }
}
