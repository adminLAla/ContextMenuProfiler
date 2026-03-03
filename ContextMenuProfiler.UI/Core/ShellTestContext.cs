using System;
using System.IO;
using ContextMenuProfiler.UI.Core.Services;

namespace ContextMenuProfiler.UI.Core
{
    /// <summary>
    /// Manages a temporary file or folder context for shell extension testing.
    /// Since we now use the Hook scheme in explorer.exe, the UI no longer needs 
    /// to handle complex COM IDataObject/PIDL creation.
    /// </summary>
    public class ShellTestContext : IDisposable
    {
        public string Path { get; private set; } = "";

        public static ShellTestContext Create(bool isFolder = false)
        {
            var context = new ShellTestContext();
            try
            {
                string tempPath;
                if (isFolder)
                {
                    tempPath = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "ContextMenuProfiler_probe_dir");
                    if (!Directory.Exists(tempPath)) Directory.CreateDirectory(tempPath);
                }
                else
                {
                    tempPath = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "ContextMenuProfiler_probe.zip");
                    if (!File.Exists(tempPath)) File.WriteAllText(tempPath, "probe");
                }
                context.Path = tempPath;
            }
            catch (Exception ex)
            {
                LogService.Instance.Warning($"Failed to create shell test context (Folder={isFolder})", ex);
            }
            return context;
        }

        public void Dispose()
        {
            // We keep the temp files to avoid locking issues with shell extensions
            // but the class is kept for future cleanup logic if needed.
        }
    }
}
