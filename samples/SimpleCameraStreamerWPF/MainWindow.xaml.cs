using System;
using System.Windows;
using System.Threading;


namespace SimpleCameraStreamerWPF
{

    public partial class MainWindow : Window
    {
        SentechCameraCoreWPF.ControlHost hwndSentechCamera;
        private uint activeCamera;

        public MainWindow()
        {
            InitializeComponent();
            activeCamera = 0;
        }

        private void On_UIReady(object sender, RoutedEventArgs e)
        {
            PresentationSource source = PresentationSource.FromVisual(this);

            hwndSentechCamera = new SentechCameraCoreWPF.ControlHost(
                ControlHostElement.ActualHeight,
                ControlHostElement.ActualWidth);

            ControlHostElement.Child = hwndSentechCamera;
            while (hwndSentechCamera.GetCameraCount() == 0) ;
            hwndSentechCamera.SetDisplayInfo(activeCamera, 0.0f, 0.0f, 0.5f, 1.0f, 0, 0);
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

        private void onSwitchCameraClick(object sender, RoutedEventArgs e)
        {
            if (activeCamera == 0)
            {
                hwndSentechCamera.SetDisplayInfo(0, 0.0f, 0.0f, 0.5f, 1.0f, 0, 255);
                hwndSentechCamera.SetDisplayInfo(1, 0.0f, 0.0f, 0.5f, 1.0f, 0, 0);
                activeCamera = 1;
            }
            else if(activeCamera == 1)
            {
                hwndSentechCamera.SetDisplayInfo(1, 0.0f, 0.0f, 0.5f, 1.0f, 0, 255);
                hwndSentechCamera.SetDisplayInfo(0, 0.0f, 0.0f, 0.5f, 1.0f, 0, 0);
                activeCamera = 0;
            }
        }
    }
}
