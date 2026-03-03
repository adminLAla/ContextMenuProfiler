using System;
using System.Windows.Threading;
using Wpf.Ui.Controls;

namespace ContextMenuProfiler.UI.Core.Services
{
    public class NotificationService
    {
        public static NotificationService Instance { get; } = new NotificationService();
        
        private SnackbarPresenter? _presenter;
        
        private NotificationService() { }

        public void Initialize(SnackbarPresenter presenter)
        {
            _presenter = presenter;
        }

        public void ShowSuccess(string title, string message)
        {
            Show(title, message, ControlAppearance.Success, SymbolRegular.CheckmarkCircle24);
        }

        public void ShowError(string title, string message)
        {
            Show(title, message, ControlAppearance.Danger, SymbolRegular.ErrorCircle24);
        }

        public void ShowInfo(string title, string message)
        {
            Show(title, message, ControlAppearance.Info, SymbolRegular.Info24);
        }

        public void ShowWarning(string title, string message)
        {
            Show(title, message, ControlAppearance.Caution, SymbolRegular.Warning24);
        }

        private void Show(string title, string message, ControlAppearance appearance, SymbolRegular icon)
        {
            if (_presenter == null) return;

            // Ensure we are on the UI thread
            if (_presenter.Dispatcher.CheckAccess())
            {
                _presenter.AddToQue(new Snackbar(_presenter)
                {
                    Title = title,
                    Content = message,
                    Appearance = appearance,
                    Icon = new SymbolIcon(icon),
                    Timeout = TimeSpan.FromSeconds(5)
                });
            }
            else
            {
                _presenter.Dispatcher.Invoke(() => Show(title, message, appearance, icon));
            }
        }
    }
}
