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
            activeCamera++;
            if(activeCamera >= hwndSentechCamera.GetCameraCount())
                activeCamera = 0;

            for(uint i = 0; i < hwndSentechCamera.GetCameraCount(); i++)
            {
                if (i == activeCamera)
                    hwndSentechCamera.SetDisplayInfo(i, 0.0f, 0.0f, 0.5f, 1.0f, 0, 0);
                else
                    hwndSentechCamera.SetDisplayInfo(i, 0.0f, 0.0f, 0.5f, 1.0f, 0, 255);
            }
        }
    }
}
