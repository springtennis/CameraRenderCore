using System;
using System.Windows;
using System.Threading;


namespace SimpleCameraStreamerWPF
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        SentechCameraCoreWPF.ControlHost hwndSentechCamera;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void On_UIReady(object sender, RoutedEventArgs e)
        {
            PresentationSource source = PresentationSource.FromVisual(this);

            hwndSentechCamera = new SentechCameraCoreWPF.ControlHost(
                ControlHostElement.ActualHeight,
                ControlHostElement.ActualWidth);

            ControlHostElement.Child = hwndSentechCamera;
            while (hwndSentechCamera.GetCameraCount() == 0) ;
            hwndSentechCamera.SetDisplayInfo(0, 0.0f, 0.0f, 0.5f, 1.0f, 0, 0);
        }

        private void onRecordClick(object sender, RoutedEventArgs e)
        {
            string videoPath = Environment.GetFolderPath(Environment.SpecialFolder.MyVideos) + "\\test.mp4";
            hwndSentechCamera.SetRecordInfo(0, videoPath);
            hwndSentechCamera.StartRecord();
        }

        private void onStopClick(object sender, RoutedEventArgs e)
        {
            hwndSentechCamera.StopRecord();
        }
    }
}
