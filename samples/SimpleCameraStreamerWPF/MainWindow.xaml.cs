using System;
using System.Windows;

namespace SimpleCameraStreamerWPF
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void On_UIReady(object sender, RoutedEventArgs e)
        {
            PresentationSource source = PresentationSource.FromVisual(this);

            SentechCameraCoreWPF.ControlHost cameraHost = new SentechCameraCoreWPF.ControlHost(
                ControlHostElement.ActualHeight,
                ControlHostElement.ActualWidth);

            ControlHostElement.Child = cameraHost;
        }
    }
}
